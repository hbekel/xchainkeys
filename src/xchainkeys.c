/* xchainkeys 0.1 -- exclusive keychains for X11
 * Copyright (C) 2010 Henning Bekel <h.bekel@googlemail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <X11/Xlib.h>

#include "key.h"
#include "binding.h"
#include "xchainkeys.h"

XChainKeys_t *xc;

XChainKeys_t* xc_new(Display *display) {
  XChainKeys_t *self = (XChainKeys_t *) calloc(1, sizeof(XChainKeys_t)); 
  self->display = display;
  self->debug = False;
  self->root = binding_new();
  self->root->action = ":root";
  return self;
}

void xc_version(XChainKeys_t *self) {
  printf("xchainkeys %s Copyright (C) 2010 Henning Bekel <%s>\n",
	 PACKAGE_VERSION, PACKAGE_BUGREPORT);
}

void xc_usage(XChainKeys_t *self) {
  printf("Usage: xchainkeys [options]\n\n");
  printf("  -d, --debug   : Enable debug messages\n");
  printf("  -h, --help    : Print this help text\n");
  printf("  -v, --version : Print version information\n");
  printf("\n");
}

void xc_parse_config(XChainKeys_t *self, char *path) {

  FILE *config;
  char buffer[4096];
  char argument[4096];
  char *line, *token, *expect;
  const char *ws = " \t"; 
  int linenum = 0;
  int len, pos, i;
  Key_t *key;
  Binding_t *binding;
  Binding_t *parent;

  config = fopen(path, "r");
  
  if(config == NULL) {
    fprintf(stderr, "error '%s': %s\n", path, strerror(errno));
    exit(EXIT_FAILURE);
  }

  while(fgets(buffer, 4096, config) != NULL) {
    linenum++;
    line = buffer;

    // discard whitespace at the beginning
    line += strspn(line, " \t");
    
    // ignore empty lines and comments
    if( line[0] == '\n' || line[0] == '#' )
      continue;

    // discard newline at end of string
    if( line[strlen(line)-1] == '\n' )
      line[strlen(line)-1] = '\0';
    
    if (self->debug) fprintf(stderr, "%s\n", line);

    pos = 0;
    parent = xc->root;
    binding = NULL;
    expect = "key";
    argument[0] = '\0';

    while(line[0] != '\0') {
      len = strcspn(line, ws);
      
      token = calloc(1, (len+1)*sizeof(char));
      strncpy(token, line, len);

      line += strcspn(line, ws);
      line += strspn(line, ws);

      if( token[0] == ':' )
	expect = "action";

      if (self->debug) fprintf(stderr, "%d: (expect %s) '%s'\n", pos, expect, token);

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

	  if(parent == xc->root && !strlen(binding->action))
	    binding->action = ":enter";

	  // make this binding the parent for the next
	  parent = binding;
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
      }

    next_token:
      pos++;
      free(token);
    }
    // done with this line
    
    // append the argument to the current binding
    if (strlen(argument))
      binding->argument = strdup(argument);

    // apply default action :enter
    if(!strlen(binding->action))
      binding->action = ":enter";
  }
  fclose(config);

  // add default :escape and :abort bindings to :enter'ing root bindings

  for (i=0; i<self->root->num_children; i++) {
    parent = self->root->children[i];
    if (strcmp(parent->action, ":enter") == 0) {
      if (!binding_get_child_by_action(parent, ":escape")) {
	binding = binding_new();
	binding->action = ":escape";
	binding->key = parent->key;
	binding_append_child(parent, binding);
      }
      if (!binding_get_child_by_action(parent, ":abort")) {
	binding = binding_new();
	binding->action = ":abort";
	binding->key = key_new("C-g");
	binding_append_child(parent, binding);
      }
    }
  }
  if (self->debug) binding_list(self->root);
}

void xc_mainloop(XChainKeys_t *self) {
  if (self->debug) fprintf(stderr, "xc_mainloop: not implemented\n"); 
}

void xc_destroy(XChainKeys_t *self) {
  free(self);
}

int main(int argc, char **argv) {

  Display *display;

  struct option options[] = {
    { "debug", no_argument, NULL, 'd' },
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'v' },
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

    option = getopt_long(argc, argv, "dhv", options, &option_index);
    
    switch (option) {

    case 'd':
      xc->debug = True;
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
  xc_parse_config(xc, "/home/chenno/workspace/xchainkeys/example.conf");

  /* enter mainloop */
  xc_mainloop(xc);

  exit(EXIT_SUCCESS);
}
