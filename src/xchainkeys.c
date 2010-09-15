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
#include <sys/time.h>
#include <X11/Xlib.h>

#include "key.h"
#include "binding.h"
#include "popup.h"
#include "xchainkeys.h"

XChainKeys_t *xc;

XChainKeys_t* xc_new(Display *display) {

  XChainKeys_t *self;
  unsigned int i, num, caps, scroll;

  self = (XChainKeys_t *) calloc(1, sizeof(XChainKeys_t)); 

  self->display = display;
  self->debug = False;
  self->timeout = 3000;
  self->delay = 1000;
  self->root = binding_new();
  self->root->action = ":root";
  self->xmodmap = XGetModifierMapping(self->display);
  
  i = 0;
  num = 
    xc_keycode_to_modmask(self, XKeysymToKeycode(self->display, XStringToKeysym("Num_Lock")));
  caps = 
    xc_keycode_to_modmask(self, XKeysymToKeycode(self->display, XStringToKeysym("Caps_Lock")));
  scroll = 
    xc_keycode_to_modmask(self, XKeysymToKeycode(self->display, XStringToKeysym("Scoll_Lock")));

  self->modmask[i++] = 0;
  self->modmask[i++] = num;
  self->modmask[i++] = caps;
  self->modmask[i++] = scroll;
  self->modmask[i++] = num | caps;
  self->modmask[i++] = num | scroll;
  self->modmask[i++] = caps | scroll;
  self->modmask[i++] = num | caps | scroll;

  xc_find_config(self);

  return self;
}

void xc_version(XChainKeys_t *self) {
  printf("%s %s Copyright (C) 2010 Henning Bekel <%s>\n",
	 PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_BUGREPORT);
}

void xc_usage(XChainKeys_t *self) {
  printf("Usage: %s [options]\n\n", PACKAGE_NAME);
  printf("  -d, --debug   : Enable debug messages\n");
  printf("  -h, --help    : Print this help text\n");
  printf("  -v, --version : Print version information\n");
  printf("  -k, --keys    : Show valid keyspecs\n");
  printf("  -f, --file    : alternative config file\n");
  printf("\n");
}

void xc_find_config(XChainKeys_t *self) {
  
  self->config = (char *) calloc(4096, sizeof(char));

  /* determine config file self->config */
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
  char *line, *token, *expect;
  const char *ws = " \t"; 
  int linenum = 0;
  int len, pos, i;

  Key_t *key;
  Binding_t *binding;
  Binding_t *parent;
  char *font = "fixed";
  char *fg = "black";
  char *bg = "white";

  /* try to open config file */
  f = fopen(self->config, "r");
  
  if(f == NULL) {
    fprintf(stderr, "error: '%s': %s\n", self->config, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (self->debug) fprintf(stderr, "Parsing config file %s...\n", self->config);

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
    
    /* if (self->debug) fprintf(stderr, "%s\n", line); */

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

    if( strncmp(line, "font", 4) == 0 ) {
      line += 4;
      line += strspn(line, ws);
      line[strcspn(line, ws)] = '\0';
      font = strdup(line);
      continue;
    }

    if( strncmp(line, "foreground", 10) == 0 ) {
      line += 10;
      line += strspn(line, ws);
      line[strcspn(line, ws)] = '\0';
      fg = strdup(line);
      continue;
    }

    if( strncmp(line, "background", 10) == 0 ) {
      line += 10;
      line += strspn(line, ws);
      line[strcspn(line, ws)] = '\0';
      bg = strdup(line);
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

      /* if (self->debug) fprintf(stderr, "%d: (expect %s) '%s'\n", pos, expect, token); */

      if (strcmp(expect, "key") == 0) {
	if ((key = key_new(token)) != NULL) {

	  // if this key is already bound in the parent binding,
	  // then make that binding the parent binding for the next key
	  if(binding_get_child_by_key(parent, key) != NULL) {
	    parent = binding_get_child_by_key(parent, key);
	    goto next_token;
	  }  

	  // create a binding for this key
	  binding = binding_new();
	  binding->key = key;

	  // append the new binding to the current parent
	  binding_append_child(parent, binding);

	  // make this binding the parent for the next
	  parent = binding;
	}
	else {
	  fprintf(stderr, 
		  "Warning: Invalid keyspec: '%s'. Use %s -k to show valid keyspecs.\n",
		  token, PACKAGE_NAME);
	  fprintf(stderr, "-> Skipping line %d: '%s'\n", linenum, buffer);
	  goto next_line;
	}
      }
      
      if (strcmp(expect, "action") == 0) {
	binding->action = strdup(token);
	expect = "argument";
	goto next_token;
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
    // done with this line
    
    if (binding != NULL) {
      // append the argument to the current binding
      if (strlen(argument)) {
	binding->argument = strdup(argument);
      }
    }
  next_line:
    ;
  }
  fclose(f);

  /* add defaults */
  for (i=0; i<self->root->num_children; i++) {
    parent = self->root->children[i];
    if (strcmp(parent->action, ":enter") == 0) {

      /* create default :escape binding unless present */
      if (!binding_get_child_by_action(parent, ":escape")) {
	binding = binding_new();
	binding->action = ":escape";
	binding->key = parent->key;
	binding_append_child(parent, binding);
      }

      /* create default :abort binding unless present */
      if (!binding_get_child_by_action(parent, ":abort")) {
	binding = binding_new();
	binding->action = ":abort";
	binding->key = key_new("C-g");
	binding_append_child(parent, binding);
      }
    }
  }

  /* initialize popup window */
  self->popup = popup_new(self->display, font, fg, bg);

  if (self->debug) {
    printf("\n");
    printf("timeout %d\n", self->timeout);
    printf("delay %d\n", self->delay);
    printf("font %s\n", font);
    printf("foreground %s\n", fg);
    printf("background %s\n\n", bg);
    
    binding_list(self->root);
    printf("\n");
  }
  free(font);
  free(fg);
  free(bg);
  free(buffer);
  free(argument);
}

void xc_mainloop(XChainKeys_t *self) {
  Binding_t *binding;
  XEvent event;
  KeyCode keycode;
  long now;
  int i;

  /* grab top level keys individually */
  for (i=0; i<self->root->num_children; i++) {
    binding = self->root->children[i];
    key_grab(binding->key);    
  }

  while(True) {

    while(!XPending(self->display)) {      
      if(xc->popup->timeout == 0)
	break;
      
      now = xc_get_msec();
      
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
	  if(binding->key->modifiers == xc_get_modifiers(xc)) {

	    popup_hide(xc->popup);
	    xc->popup->timeout = 0;
	    
	    binding_activate(binding);
	    break;
	  }
	}
      }
    }
  }
}

void xc_show_keys(XChainKeys_t *self) {

  XEvent event;
  Key_t *key;
  KeyCode keycode;
  char *keystr;
  char *keyspec;

  xc_version(xc);
  printf("\nPress a key combination to show the corresponding keyspec.\n");
  printf("Press Control-c to quit.\n");
  
  XGrabKeyboard(self->display, DefaultRootWindow(self->display),
		True, GrabModeAsync, GrabModeAsync, CurrentTime);

  while(True) {
    XNextEvent(self->display, &event);
    switch(event.type) {
    case KeyPress:
      
      keycode = ((XKeyPressedEvent*)&event)->keycode;
      keystr = XKeysymToString(XKeycodeToKeysym(self->display, keycode, 0));
      
      if (xc_keycode_to_modmask(xc, keycode) != 0) 
	continue;
      
      if ( strcmp(keystr, "c") == 0 && xc_get_modifiers(self) == ControlMask ) {
	XUngrabKeyboard(self->display, CurrentTime);
	return;
      }
      
      key = key_new(keystr);      
      key->modifiers = xc_get_modifiers(self);
      
      keyspec = key_to_str(key);
      printf("%s\n", keyspec);
      free(keyspec);
      free(key);
    }
  }
}

int xc_keycode_to_modmask(XChainKeys_t *self, KeyCode keycode) {

  int i = 0;
  int j = 0;
  int max = self->xmodmap->max_keypermod;

  for (i = 0; i < 8; i++) {
    for (j = 0; j < max && self->xmodmap->modifiermap[(i * max) + j]; j++) {
      if (keycode == self->xmodmap->modifiermap[(i * max) + j]) {
        switch (i) {
          case ShiftMapIndex: return ShiftMask; break;
          case LockMapIndex: return LockMask; break;
          case ControlMapIndex: return ControlMask; break;
          case Mod1MapIndex: return Mod1Mask; break;
          case Mod2MapIndex: return Mod2Mask; break;
          case Mod3MapIndex: return Mod3Mask; break;
          case Mod4MapIndex: return Mod4Mask; break;
          case Mod5MapIndex: return Mod5Mask; break;
        }
      } 
    } 
  } 
  return 0;
}

unsigned int xc_get_modifiers(XChainKeys_t *self) {

  char keymap[32]; 
  unsigned int keycode;
  unsigned int modifiers = 0;

  XQueryKeymap(self->display, keymap);

  for (keycode = 0; keycode < 256; keycode++) {
    if ((keymap[(keycode / 8)] & (1 << (keycode % 8))) \
        && xc_keycode_to_modmask(self, keycode)) {

      modifiers |= xc_keycode_to_modmask(self, keycode);
    }
  }
  return modifiers;
}

long xc_get_msec() {
  long msec;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  msec = (long)(tv.tv_sec*1000 + (tv.tv_usec/1000));
  return msec;
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

  /* try to open the x display */
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
      xc_version(xc);
      break;
      
    case 'h':
      xc_usage(xc);
      exit(EXIT_SUCCESS);
      break;
      
    case 'v':
      xc_version(xc);
      exit(EXIT_SUCCESS);
      break;      

    case -1:
      break;

    default:
      xc_usage(xc);
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

