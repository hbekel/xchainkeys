/* xchainkeys -- chained keybindings for X11
 * Copyright (C) 2010 Henning Bekel <h.bekel@googlemail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.  
 */

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif /* _XOPEN_SOURCE */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <X11/Xlib.h>

#include "key.h"
#include "binding.h"
#include "popup.h"
#include "util.h"
#include "xchainkeys.h"

XChainKeys_t *xc;

XChainKeys_t* xc_new() {

  XChainKeys_t *self = (XChainKeys_t *) calloc(1, sizeof(XChainKeys_t)); 

  if(NULL == (self->display=XOpenDisplay(NULL))) {
    
    fprintf(stderr, "%s: error: XOpenDisplay() failed for DISPLAY=%s.\n", 
	    PACKAGE_NAME, getenv("DISPLAY")); 
    fflush(stderr);
    
    free(self);
    exit(EXIT_FAILURE);
  }

  self->debug = False;
  self->timeout = 3000;
  self->delay = 1000;
  self->reentry = NULL;
  self->reload = False;

  self->xmodmap = XGetModifierMapping(self->display);
  xc_init_modmask(self);

  self->action_names[0] = ":none";
  self->action_names[1] = ":enter";
  self->action_names[2] = ":escape";
  self->action_names[3] = ":abort";
  self->action_names[4] = ":exec";
  self->action_names[5] = ":group";
  self->action_names[6] = ":load";

  self->root = binding_new();
  self->root->action = XC_ACTION_NONE;
  
  xc_find_config(self);

  return self;
}

void xc_init_modmask(XChainKeys_t *self) {
  unsigned int num, caps, scroll;

  /* self->modmask contains all possible combinations of num, caps and
   * scroll lock */

  num = 
    keycode_to_modifier(self->xmodmap, 
			XKeysymToKeycode(self->display, XStringToKeysym("Num_Lock")));
  caps = 
    keycode_to_modifier(self->xmodmap, 
			XKeysymToKeycode(self->display, XStringToKeysym("Caps_Lock")));
  scroll = 
    keycode_to_modifier(self->xmodmap, 
			XKeysymToKeycode(self->display, XStringToKeysym("Scoll_Lock")));

  self->modmask[0] = 0;
  self->modmask[1] = num;
  self->modmask[2] = caps;
  self->modmask[3] = scroll;
  self->modmask[4] = num | caps;
  self->modmask[5] = num | scroll;
  self->modmask[6] = caps | scroll;
  self->modmask[7] = num | caps | scroll;
}

void xc_show_keys(XChainKeys_t *self) {

  XEvent event;
  Key_t *key;
  KeyCode keycode;
  char *keystr;
  char *keyspec;

  version();
  printf("\nPress a key combination to show the corresponding keyspec.\n");
  printf("Press Control-c to quit.\n\n");
  fflush(stdout);

  XGrabKeyboard(self->display, DefaultRootWindow(self->display),
		True, GrabModeAsync, GrabModeAsync, CurrentTime);

  while(True) {
    XNextEvent(self->display, &event);
    switch(event.type) {
    case KeyPress:
      
      keycode = ((XKeyPressedEvent*)&event)->keycode;
      keystr = XKeysymToString(XKeycodeToKeysym(self->display, keycode, 0));
      
      if (keycode_to_modifier(xc->xmodmap, keycode) != 0) 
	continue;
      
      if ( strcmp(keystr, "c") == 0 && get_modifiers(self->display) == ControlMask ) {
	XUngrabKeyboard(self->display, CurrentTime);
	return;
      }
      
      key = key_new(keystr);      
      key->modifiers = get_modifiers(self->display);
      
      keyspec = key_to_str(key);

      printf("%s\n", keyspec);
      fflush(stdout);

      free(keyspec);
      free(key);
    }
  }
}

void xc_find_config(XChainKeys_t *self) {
  
  int n = 4096;
  self->config = (char *) calloc(n, sizeof(char));
  
  strncpy(self->config, getenv("HOME"), n);
  
  if(getenv("XDG_CONFIG_HOME") != NULL ) {
    strncpy(self->config, getenv("XDG_CONFIG_HOME"), n);
    strcat(self->config, "/xchainkeys/xchainkeys.conf");
  } 
  else {
    strcat(self->config, "/.config/xchainkeys/xchainkeys.conf");
  }
}

void xc_parse_config(XChainKeys_t *self) {

  FILE *f;
  char *buffer = (char *) calloc(4096, sizeof(char));
  char *argument= (char *) calloc(4096, sizeof(char));
  char *argument_ptr = argument;
  char *line, *token, *expect, *path;
  const char *ws = " \t"; 
  int linenum = 0;
  int len, pos;

  Key_t *key;
  Binding_t *binding;
  Binding_t *parent;
  Binding_t *existing;

  int feedback = True;

  char *font = calloc(512, sizeof(char));
  strcpy(font, "fixed");

  char *fg = calloc(64, sizeof(char));
  strcpy(fg, "black");

  char *bg = calloc(64, sizeof(char));
  strcpy(bg, "white");

  /* try to open config file */
  f = fopen(self->config, "r");
  
  if(f == NULL) {
    fprintf(stderr, "%s: error: '%s': %s\n", 
	    PACKAGE_NAME, self->config, strerror(errno));
    fflush(stderr);
    exit(EXIT_FAILURE);
  }

  if (self->debug) { 
    printf("Parsing config file %s\n", self->config);
    fflush(stdout);
  }

  /* parse file */
  while(fgets(buffer, 4096, f) != NULL) {
    linenum++;
    line = buffer;

    /* discard whitespace at the beginning */
    line += strspn(line, ws);
    
    /* ignore empty lines and comments */
    if( line[0] == '\n' || line[0] == '#' )
      continue;

    /* discard newline at end of string */
    if( line[strlen(line)-1] == '\n' )
      line[strlen(line)-1] = '\0';

    /* parse options */
    if( strncmp(line, "timeout", 7) == 0 ) {
      line += 7;
      line += strspn(line, ws);
      line[strcspn(line, ws)] = '\0';
      self->timeout = (unsigned int)atoi(line);
      continue;
    }

    if( strncmp(line, "delay", 5) == 0 ) {
      line += 5;
      line += strspn(line, ws);
      line[strcspn(line, ws)] = '\0';
      self->delay = (unsigned int)atoi(line);
      continue;
    }

    if( strncmp(line, "feedback", 8) == 0 ) {
      line += 8;
      line += strspn(line, ws);
      line[strcspn(line, ws)] = '\0';

      if(strcmp(line, "off") == 0)
	feedback = False;
      if(strcmp(line, "on") == 0) 
        feedback = True;

      continue;
    }

    if( strncmp(line, "font", 4) == 0 ) {
      line += 4;
      line += strspn(line, ws);
      line[strcspn(line, ws)] = '\0';
      strncpy(font, line, 512);
      font[strlen(line)] = '\0';
      continue;
    }

    if( strncmp(line, "foreground", 10) == 0 ) {
      line += 10;
      line += strspn(line, ws);
      line[strcspn(line, ws)] = '\0';
      strncpy(fg, line, 64);
      fg[strlen(line)] = '\0';
      continue;
    }

    if( strncmp(line, "background", 10) == 0 ) {
      line += 10;
      line += strspn(line, ws);
      line[strcspn(line, ws)] = '\0';
      strncpy(bg, line, 64);
      bg[strlen(line)] = '\0';
      continue;
    }

    /* parse bindings */

    pos = 0;
    parent = xc->root;
    binding = NULL;
    expect = "key";
    argument[0] = '\0';

    while(strlen(line)) {
      len = strcspn(line, ws);
      
      token = calloc((len+1), sizeof(char));
      strncpy(token, line, len);

      line += strcspn(line, ws);
      line += strspn(line, ws);

      if( token[0] == ':' )
	expect = "action";

      if (strcmp(expect, "key") == 0) {
	if ((key = key_new(token)) != NULL) {

	  /* if this key is already bound in the parent binding,
	   * then make that binding the parent binding for the next key
	   */
	  existing = binding_get_child_by_key(parent, key);

	  if( existing != NULL) {
	    if(line[0] == ':' || line[0] == '\0') {
	      path = binding_to_path(existing);
	      fprintf(stderr, "%s: line %d: '%s': already bound, skipping...\n",
		      PACKAGE_NAME, linenum, path);
	      fflush(stderr);
	      free(path);
	      free(key);
	      free(token);
	      goto next_line;
	    }
	    else {
	      parent = binding_get_child_by_key(parent, key);
	      free(key);
	      goto next_token;
	    }
	  }
	  
	  /* avoid binding :abort, :escape or :group at toplevel */
	  if(parent == xc->root) {
	    if(strncmp(line, ":abort", 6) == 0 ||
	       strncmp(line, ":escape", 7) == 0 ||
	       strncmp(line, ":group", 6) == 0 ) {

	      fprintf(stderr, "%s: line %d: '%s': " 
		      "action is invalid outside of chain, skipping...\n",
		      PACKAGE_NAME, linenum, line); 
	      fflush(stderr);
	      free(key);
	      free(token);
	      goto next_line;
	    }
	  }

	  /* create a binding for this key */
	  binding = binding_new();
	  binding->key = key;

	  /* append the new binding to the current parent */
	  binding_append_child(parent, binding);

	  /* make this binding the parent for the next */
	  parent = binding;
	}
	else {
	  fprintf(stderr,
		  "%s: line %d: '%s': invalid keyspec, skipping...\n",
		  PACKAGE_NAME, linenum, token);
	  fflush(stderr);
	  free(token);
	  goto next_line;
	}
      }
      
      if (strcmp(expect, "action") == 0) {
	binding_set_action(binding, token);
	expect = "argument";
	goto next_token;
      }

      if (strcmp(expect, "argument") == 0) {
	if(strlen(argument))
	  strcat(argument, " ");
	strncat(argument, token, 4096-strlen(argument)-1);
	goto next_token;
      }

    next_token:
      pos++;
      free(token);
    }
    /* all tokens parsed */
    
    if (binding != NULL) {
      
      /* parse the name portion of the argument */
      if(argument[0] == '"') {
	argument += 1;
	if(strrchr(argument, '"') == NULL) {
	  fprintf(stderr, 
		  "%s: line %d: missing closing double quote "
		  "for action name, ignoring arguments...\n",
		  PACKAGE_NAME, linenum);
	  fflush(stderr);
	  goto next_line;
	}
        len = strcspn(argument, "\"");
	strncpy(binding->name, argument, 128);
	binding->name[len] = '\0';
	argument += len + 1;
	argument += strspn(argument, ws);
      }

      /* append the argument to the current binding */
      if (strlen(argument)) {
	strncpy(binding->argument, argument, 4096);
      }
      argument = argument_ptr;
    }
  next_line:
    ;
  }
  fclose(f);
  
  binding_parse_arguments(self->root);
  binding_create_default_bindings(self->root);

  /* initialize popup window */
  self->popup = popup_new(self->display, font, fg, bg);
  self->popup->enabled = feedback;

  if (self->debug) {
    printf("\n");
    printf("timeout %d\n", self->timeout);

    if (feedback) { 
      printf("feedback on\n");
      printf("delay %d\n", self->delay);
      printf("font %s\n", font);
      printf("foreground %s\n", fg);
      printf("background %s\n\n", bg);
    }
    else {
      printf("feedback off\n\n");
    }
    
    binding_list(self->root);
    printf("\n");
    fflush(stdout);
  }
  free(buffer);
  free(argument);
  free(font);
  free(fg);
  free(bg);
}

void xc_grab_prefix_keys(XChainKeys_t *self) {
  /* grab top level keys individually */
  int i;
  for (i=0; i<self->root->num_children; i++) {
    key_grab(self->root->children[i]->key);    
  }
}

void xc_mainloop(XChainKeys_t *self) {
  Binding_t *binding;
  Binding_t *reentry;
  XEvent event;
  KeyCode keycode;
  long now;
  int i;

  xc_grab_prefix_keys(self);

  while(True) {

    while(!XPending(self->display)) {      
      if(xc->popup->timeout == 0)
	break;
      
      now = get_msec();
      
      if(now >= xc->popup->timeout) {
	popup_hide(xc->popup);
	xc->popup->timeout = 0;
      }
    }
    
    XNextEvent(self->display, &event);

    if (event.type == KeyPress) {
      keycode = ((XKeyPressedEvent*)&event)->keycode;
      
      for( i=0; i<self->root->num_children; i++ ) {
	binding = self->root->children[i];
	
	if (binding->key->keycode == keycode) {
	  if(binding->key->modifiers == get_modifiers(xc->display)) {

	    popup_hide(xc->popup);
	    xc->popup->timeout = 0;
	    
	    binding_activate(binding);
	    break;
	  }
	}
      }
    }
  reentry:
    if(xc->reentry != NULL) {
      reentry = xc->reentry;
      xc->reentry = NULL;
      binding_activate(reentry);
      goto reentry;
    }

    if(xc->reload) {
      xc_reload(self);
    }
  }
}

void xc_reset(XChainKeys_t *self) {
  int i;
  Binding_t *prefix;

  for(i=0; i<self->root->num_children; i++) {
    prefix = self->root->children[i];

    key_ungrab(prefix->key);
    binding_free(prefix);
    self->root->children[i] = NULL;
  }
  self->root->num_children = 0;

  self->reentry = NULL;
  self->reload = False;

  popup_free(self->popup);
  self->popup = NULL;
}

void xc_reload(XChainKeys_t *self) {  
  xc_reset(self);
  xc_parse_config(self);
  xc_grab_prefix_keys(self);
}

void xc_parse_options(XChainKeys_t *self, int argc, char **argv) {

  struct option options[] = {
    { "debug", no_argument, NULL, 'd' },
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'v' },
    { "keys", no_argument, NULL, 'k' },
    { "file", no_argument, NULL, 'f' },
    { 0, 0, 0, 0 },
  };
  int option, option_index;

  while (1) {

    option = getopt_long(argc, argv, "dhvkf:", options, &option_index);
    
    switch (option) {

    case 'f':
      strncpy(self->config, optarg, 4096);
      self->config[strlen(optarg)] = '\0';
      break;      

    case 'k':
      xc_show_keys(self);
      exit(EXIT_SUCCESS);

    case 'd':
      self->debug = True;
      version();
      printf("\n"); fflush(stdout);
      break;
      
    case 'h':
      usage();
      exit(EXIT_SUCCESS);
      break;
      
    case 'v':
      version();
      exit(EXIT_SUCCESS);
      break;      

    case -1:
      break;

    default:
      usage();
      exit(EXIT_FAILURE);
      break;
    }
    if(option == -1)
      break;
  }  
}

int main(int argc, char **argv) {

  xc = xc_new();

  xc_parse_options(xc, argc, argv);
  xc_parse_config(xc);
  xc_mainloop(xc);

  exit(EXIT_SUCCESS);
}
