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

XChainKeys_t* xc_new(Display *display) {

  XChainKeys_t *self;

  self = (XChainKeys_t *) calloc(1, sizeof(XChainKeys_t)); 

  self->display = display;
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
  self->num_actions = 7;

  self->root = binding_new();
  self->root->action = XC_ACTION_NONE;
  
  xc_find_config(self);

  return self;
}

void xc_init_modmask(XChainKeys_t *self) {
  int i=0;
  unsigned int num, caps, scroll;

  /* prepare modmask array, containing all possible combinations of
   * num, caps and scroll lock */

  num = 
    keycode_to_modifier(self->xmodmap, XKeysymToKeycode(self->display, XStringToKeysym("Num_Lock")));
  caps = 
    keycode_to_modifier(self->xmodmap, XKeysymToKeycode(self->display, XStringToKeysym("Caps_Lock")));
  scroll = 
    keycode_to_modifier(self->xmodmap, XKeysymToKeycode(self->display, XStringToKeysym("Scoll_Lock")));

  i = 0;
  self->modmask[i++] = 0;
  self->modmask[i++] = num;
  self->modmask[i++] = caps;
  self->modmask[i++] = scroll;
  self->modmask[i++] = num | caps;
  self->modmask[i++] = num | scroll;
  self->modmask[i++] = caps | scroll;
  self->modmask[i++] = num | caps | scroll;
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
      free(keyspec);
      free(key);
    }
  }
}

void xc_find_config(XChainKeys_t *self) {
  
  self->config = (char *) calloc(4096, sizeof(char));

  strcpy(self->config, getenv("HOME"));
  
  if(getenv("XDG_CONFIG_HOME") != NULL ) {
    strcpy(self->config, getenv("XDG_CONFIG_HOME"));
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
    exit(EXIT_FAILURE);
  }

  if (self->debug) printf("Parsing config file %s\n", self->config);

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
      strncpy(font, line, strlen(line));
      font[strlen(line)] = '\0';
      continue;
    }

    if( strncmp(line, "foreground", 10) == 0 ) {
      line += 10;
      line += strspn(line, ws);
      line[strcspn(line, ws)] = '\0';
      strncpy(fg, line, strlen(line));
      fg[strlen(line)] = '\0';
      continue;
    }

    if( strncmp(line, "background", 10) == 0 ) {
      line += 10;
      line += strspn(line, ws);
      line[strcspn(line, ws)] = '\0';
      strncpy(bg, line, strlen(line));
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
	      fprintf(stderr, "%s: warning: line %d: %s: already bound, skipping...\n",
		      PACKAGE_NAME, linenum, path);
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
		  "%s: warning: line %d: %s: invalid keyspec, skipping...\n",
		  PACKAGE_NAME, linenum, token);
	  free(token);
	  goto next_line;
	}
      }
      
      if (strcmp(expect, "action") == 0) {
	binding_set_action(binding, token);
	expect = "name";
	goto next_token;
      }

      if (strcmp(expect, "name") == 0) {
	if( token[0] == '"' && token[strlen(token)-1] == '"' ) {
	  token += 1; 
	  token[strlen(token)-1] = '\0';

	  strcpy(binding->name, token);
	  token -= 1;
	  expect = "argument";
	  goto next_token;
	}
	else {
	  expect = "argument";
	}
      }

      if (strcmp(expect, "argument") == 0) {
	if(strlen(argument))
	  strncat(argument, " ", 1);
	strncat(argument, token, strlen(token));
	goto next_token;
      }

    next_token:
      pos++;
      free(token);
    }
    /* all tokens parsed */
    
    if (binding != NULL) {
      /* append the argument to the current binding */
      if (strlen(argument)) {
	strcpy(binding->argument, argument);
      }
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

int main(int argc, char **argv) {

  Display *display;

  struct option options[] = {
    { "debug", no_argument, NULL, 'd' },
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'v' },
    { "keys", no_argument, NULL, 'k' },
    { "file", no_argument, NULL, 'f' },
    { 0, 0, 0, 0 },
  };
  int option, option_index;

  /* try to open a connection to the X display */
  if (NULL==(display=XOpenDisplay(NULL))) {
    perror(argv[0]);
    exit(EXIT_FAILURE);
  }

  /* create XChainkeys instance */
  xc = xc_new(display);
 
  /* parse command line arguments */
  while (1) {

    option = getopt_long(argc, argv, "dhvkf:", options, &option_index);
    
    switch (option) {

    case 'f':
      strncpy(xc->config, optarg, strlen(optarg));
      xc->config[strlen(optarg)] = '\0';
      break;      

    case 'k':
      xc_show_keys(xc);
      exit(EXIT_SUCCESS);

    case 'd':
      xc->debug = True;
      version();
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

  /* parse config file */
  xc_parse_config(xc);

  /* enter mainloop */
  xc_mainloop(xc);

  exit(EXIT_SUCCESS);
}
