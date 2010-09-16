#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif /* _XOPEN_SOURCE */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <X11/Xlib.h>

#include "key.h"
#include "binding.h"
#include "popup.h"
#include "xchainkeys.h"

extern XChainKeys_t *xc;

Binding_t* binding_new() {
  Binding_t *self = (Binding_t *) calloc(1, sizeof(Binding_t));

  self->key = NULL;
  self->action = XC_ACTION_ENTER;
  self->argument = "";

  self->timeout = 3000;
  self->abort = XC_ABORT_AUTO;

  self->parent = NULL;
  self->num_children = 0;

  return self;
}

void binding_set_action(Binding_t *self, char *str) {
  int i;
  for( i=0; i<xc->num_actions; i++) {
    if(strncmp(xc->action_names[i], str, strlen(str)) == 0) {
      self->action = i;
      break;
    }
  }
}

void binding_parse_arguments(Binding_t *self) {

  char *argument;
  char *argument_ptr;
  char *value;
  char *value_ptr;

  char *ws = " \t";
  int i;
  
  if(self->action == XC_ACTION_ENTER) {
  
    /* initially set the timeout from the (now parsed) global timeout */
    self->timeout = xc->timeout;
  
    argument = (char *) calloc(strlen(self->argument)+1, sizeof(char));
    argument_ptr = argument;

    strncpy(argument, self->argument, strlen(self->argument));

    while(strlen(argument)) {
      if(strncmp(argument, "timeout=", 8) == 0) {

	value = (char *) calloc(strlen(argument)+1, sizeof(char));
	value_ptr = value;

	strncpy(value, argument, strlen(argument));
	value += 8;
	value[strcspn(value, ws)] = '\0';
	
	self->timeout = atoi(value);

	argument += 8 + strlen(value);
	argument += strspn(argument, ws);

	free(value_ptr);
      }

      if(strncmp(argument, "abort=", 6) == 0) {

	value = (char *) calloc(strlen(argument)+1, sizeof(char));
	value_ptr = value;

	strncpy(value, argument, strlen(argument));
	value += 6;
	value[strcspn(value, ws)] = '\0';
	
	if(strcmp(value, "auto") == 0) 
	  self->abort = XC_ABORT_AUTO;

	if(strcmp(value, "manual") == 0)
	  self->abort = XC_ABORT_MANUAL;

	argument += 6 + strlen(value);
	argument += strspn(argument, ws);

	free(value_ptr);
      }

      argument+=strcspn(argument, ws);
      if(strcspn(argument, ws) == 0)
	break;
    }
    free(argument_ptr);
  }

  /* recurse into children and parse their arguments as well */
  for( i=0; i<self->num_children; i++ ) {
    binding_parse_arguments(self->children[i]);
  }
}

void binding_append_child(Binding_t *self, Binding_t *child) {
  self->children[self->num_children] = child;
  child->parent = self;
  self->num_children += 1;
}

Binding_t *binding_get_child_by_key(Binding_t *self, Key_t *key) {
  int i;
  for( i=0; i<self->num_children; i++ ) {
    if (key_equals(self->children[i]->key, key))
      return self->children[i];
  }	   
  return NULL;
}

Binding_t *binding_get_child_by_action(Binding_t *self, int action) {
  int i;
  for( i=0; i<self->num_children; i++ ) {
    if (self->children[i]->action == action)
      return self->children[i];
  }	   
  return NULL;
}

void binding_activate(Binding_t *self) {
  char *path;
  path = binding_to_path(self);

  if (xc->debug) 
    printf(" -> %s %s %s\n", 
	    path, xc->action_names[self->action], self->argument);

  switch(self->action) {

  case XC_ACTION_ENTER:
    binding_enter(self);
    break;

  case XC_ACTION_ESCAPE:
    binding_escape(self);
    break;

  case XC_ACTION_EXEC:    
    binding_exec(self);
    break;

  case XC_ACTION_REPEAT:
    binding_repeat(self);
    break;

  case XC_ACTION_SEND:
    binding_send(self);
    break;
  }
  free(path);
}

void binding_enter(Binding_t *self) {
  XEvent event;
  KeyCode keycode;
  Binding_t *binding;
  Key_t *key;
  char *keystr;
  char *keyspec;
  int done = False;
  long now, timeout, delay;
  char *path = binding_to_path(self);

  /* prepare timeout and popup delay */
  now = xc_get_msec();
  delay = now + xc->delay;
  timeout = xc_get_msec() + self->timeout;

  /* prepare popup */
  snprintf(xc->popup->text, strlen(path)+1, "%s", path);

  if(xc->popup->mapped)
    popup_show(xc->popup);

  /* get exclusive grab on keyboard... */
  if (self->parent == xc->root) {
    XGrabKeyboard(xc->display, DefaultRootWindow(xc->display),
		  True, GrabModeAsync, GrabModeAsync, CurrentTime);
  }

  while(!done) {    
    now = xc_get_msec();

    /* show popup after delay */
    if(now >= delay && !xc->popup->mapped)
      popup_show(xc->popup);

    /* abort on timeout */
    if(self->timeout > 0) {
      
      if(now >= timeout && self->timeout > 0) {
	if (xc->debug) printf("Timed out\n");
	done = True;
	continue;
      }
    }

    /* look for key press events... */
    if(XPending(xc->display)) {
      XNextEvent(xc->display, &event);
      
      /* dispatch exec, abort or escape */
      if(event.type == KeyPress) {

	/* get keycode and keystr */
	keycode = ((XKeyPressedEvent*)&event)->keycode;
	keystr = XKeysymToString(XKeycodeToKeysym(xc->display, keycode, 0));
	
	/* check if this key is a modifier */
	if (xc_keycode_to_modmask(xc, keycode) != 0) {
	  continue;
	}      
	else {	
	  /* non-modifier key hit... */
	  key = key_new(keystr);
	  key->modifiers = xc_get_modifiers(xc);
	  
	  /* check if this key is bound in this keymap */
	  if( (binding = binding_get_child_by_key(self, key)) != NULL) {
	    
	    /* :abort from here... */
	    if (binding->action == XC_ACTION_ABORT) {
	      if (xc->debug) printf("Aborted\n");
	      done = True;
	      free(key);
	      continue;
	    }
	    
	    /* ... or activate the binding */
	    binding_activate(binding);
	  }
	  else {
	    keyspec = key_to_str(key);
	    sprintf(xc->popup->text, "%s %s : no binding", path, keyspec);
	    popup_show(xc->popup);
	    popup_set_timeout(xc->popup, xc->delay);

	    if (xc->debug) 
	      printf(" -> %s %s: no binding\n", path, keyspec);
	    free(keyspec);
	  }	

	  /* done, exit this keymap unless manual abort was requested */
	  if (self->abort == XC_ABORT_AUTO)
	    done = True;
	  
	  free(key);
	}
      }
    }
  }
  
  /* ungrab keyboard... */
  if (self->parent == xc->root) {
    XUngrabKeyboard(xc->display, CurrentTime);
  }

  /* hide popup if no timeout is set for it */
  if(xc->popup->timeout == 0)
    popup_hide(xc->popup);

  free(path);
}

void binding_escape(Binding_t *self) {

  Window window;
  int focus_ret;

  if(self->parent == NULL)
    return;

  XUngrabKeyboard(xc->display, CurrentTime);
  XGetInputFocus(xc->display, &window, &focus_ret);

  xc_send_key(xc, self->parent->key, window);

  XGrabKeyboard(xc->display, DefaultRootWindow(xc->display),
		True, GrabModeAsync, GrabModeAsync, CurrentTime);
}

void binding_send(Binding_t *self) {

  Key_t *key;
  Window root = DefaultRootWindow(xc->display);

  key = key_new(self->argument);

  if (key != NULL) {

    XUngrabKeyboard(xc->display, CurrentTime);

    xc_send_key(xc, key, root);

    XGrabKeyboard(xc->display, DefaultRootWindow(xc->display),
		  True, GrabModeAsync, GrabModeAsync, CurrentTime);
    free(key);
  }
}

void binding_repeat(Binding_t *self) {
  Key_t *key;
  Binding_t *binding;
  XEvent event;
  KeyCode keycode;
  char *keystr;
  int i;
  int abort = False;
  
  popup_show(xc->popup);
  binding_exec(self);

  while(True) {
    XNextEvent(xc->display, &event);
    
    switch(event.type) {
    case KeyPress:
      
      /* get keycode and keystr */
      keycode = ((XKeyPressedEvent*)&event)->keycode;
      keystr = XKeysymToString(XKeycodeToKeysym(xc->display, keycode, 0));
      
      /* check if this key is a modifier */
      if (xc_keycode_to_modmask(xc, keycode) != 0) {
	break;
      }      
      else {	
	/* non-modifier key hit... */
	key = key_new(keystr);
	key->modifiers = xc_get_modifiers(xc);
	
	/* :exec all :repeat actions in the parent binding 
	 * abort on any non-repeating key
	 */
	for(i=0; i<self->parent->num_children; i++) {
	  binding = self->parent->children[i];

	  if ( binding->action == XC_ACTION_REPEAT) { 
	    if ( key_equals(binding->key, key) ) {
	      binding_exec(binding);
	      abort = False;
	      break;
	    }
	  }
	  abort = True;
	}
	if(abort) {
	  /* if the aborting key matches any root :enter bindings,
	   * prepare immediate re-entry into that chain from
	   * xc_mainloop()
	   */

	  for(i=0; i<xc->root->num_children; i++) {
	    binding = xc->root->children[i];
	    if ( binding->action == XC_ACTION_ENTER &&
		 key_equals(binding->key, key) ) {
	      xc->reentry = binding;
	    }
	  } 
	  free(key);
	  return;
	}
	free(key);
      }
    }
  }
}

void binding_exec(Binding_t *self) {
  pid_t pid;
  char *command = self->argument;

  if (!(pid = fork())) {
    setsid();
    switch (fork())
      {
      case 0: execlp ("sh", "sh", "-c", command, (char *) NULL);
	break;
      default: _exit(0);
	break;
      }
  }
  if (pid > 0) wait(NULL);
}

char *binding_to_path(Binding_t *self) {
  Binding_t *binding = self;
  char *path = (char *) calloc(4096, sizeof(char));
  char *keystr;
  do {
    keystr = key_to_str(binding->key);
    if(strlen(path) > 0)
      strncat(keystr, " ", 1);

    memmove(path+strlen(keystr), path, strlen(path));
    memmove(path, keystr, strlen(keystr));
    
    free(keystr);

    binding = binding->parent;
  } while(binding->parent != NULL);
  return path;
}

void binding_list(Binding_t *self) {
  Binding_t *current;
  char *keyspec;
  int depth = 0;
  int i;
  
  current = self;
  while( current->parent != NULL ) {
    depth++;
    current = current->parent;      
  }
  for( i=1; i<depth; i++ )
    printf("    ");  

  if(depth > 0) {
    keyspec = key_to_str(self->key);
    printf("%s %s %s\n", 
	    keyspec, xc->action_names[self->action], self->argument);
    free(keyspec);
  }

  for( i=0; i<self->num_children; i++ ) {
    binding_list(self->children[i]);
  }
}
