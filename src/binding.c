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
  self->action = ":enter";
  self->argument = "";

  self->parent = NULL;
  self->num_children = 0;

  return self;
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

Binding_t *binding_get_child_by_action(Binding_t *self, char *action) {
  int i;
  for( i=0; i<self->num_children; i++ ) {
    if (strcmp(self->children[i]->action, action) == 0)
      return self->children[i];
  }	   
  return NULL;
}

void binding_activate(Binding_t *self) {
  char *path;
  path = binding_to_path(self);

  if (xc->debug) 
    fprintf(stderr, " -> %s %s %s\n", 
	    path, self->action, self->argument);

  if(strcmp(self->action, ":enter") == 0)
    binding_enter(self);

  if(strcmp(self->action, ":escape") == 0)
    binding_escape(self);

  if(strcmp(self->action, ":exec") == 0)
    binding_exec(self);

  if(strcmp(self->action, ":repeat") == 0)
    binding_repeat(self);

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

  // prepare timeout and popup delay
  now = xc_get_msec();
  delay = now + xc->delay;
  timeout = xc_get_msec() + xc->timeout;

  // prepare popup
  snprintf(xc->popup->text, strlen(path)+1, "%s", path);

  if(xc->popup->mapped)
    popup_show(xc->popup);

  // get exclusive grab on keyboard...
  if (self->parent == xc->root) {
    XGrabKeyboard(xc->display, DefaultRootWindow(xc->display),
		  True, GrabModeAsync, GrabModeAsync, CurrentTime);
  }

  while(!done) {    
    now = xc_get_msec();

    // show popup after delay
    if(now >= delay && !xc->popup->mapped)
      popup_show(xc->popup);

    // abort on timeout (unless in chroot)
    if(strncmp(self->argument, "chroot", 6) != 0) {
      
      if(now >= timeout && xc->timeout > 0) {
	if (xc->debug) fprintf(stderr, "Timed out\n");
	done = True;
	continue;
      }
    }

    // look for key press events...
    if(XPending(xc->display)) {
      XNextEvent(xc->display, &event);
      
      // dispatch exec, abort or escape  
      switch(event.type) {
      case KeyPress:
	
	// get keycode and keystr
	keycode = ((XKeyPressedEvent*)&event)->keycode;
	keystr = XKeysymToString(XKeycodeToKeysym(xc->display, keycode, 0));
	
	// check if this key is a modifier 
	if (xc_keycode_to_modmask(xc, keycode) != 0) {
	  break;
	}      
	else {	
	  // non-modifier key hit...
	  key = key_new(keystr);
	  key->modifiers = xc_get_modifiers(xc);
	  
	  // check if this key is bound in this keymap
	  if( (binding = binding_get_child_by_key(self, key)) != NULL) {
	    
	    // :abort from here...
	    if (strcmp(binding->action, ":abort") == 0) {
	      if (xc->debug) fprintf(stderr, "Aborted\n");
	      done = True;
	      break;
	    }
	    
	    // ... or activate the binding
	    binding_activate(binding);
	  }
	  else {
	    keyspec = key_to_str(key);
	    sprintf(xc->popup->text, "%s %s : no binding", path, keyspec);
	    popup_show(xc->popup);
	    popup_set_timeout(xc->popup, xc->delay);

	    if (xc->debug) 
	      fprintf(stderr, " -> %s %s: no binding\n", path, keyspec);
	    free(keyspec);
	  }	

	  // done, exit this keymap unless we're in a chroot
	  if(strncmp(self->argument, "chroot", 6) != 0)
	    done = True;

	  free(key);
	  break;
	}
      }
    }
  }

  // ungrab keyboard...
  if (self->parent == xc->root) {
    XUngrabKeyboard(xc->display, CurrentTime);
  }

  // hide popup if no timeout is set for it
  if(xc->popup->timeout == 0)
    popup_hide(xc->popup);

  free(path);
}

void binding_escape(Binding_t *self) {

  XKeyEvent e;
  Window focused;
  int focus_ret;
  char *keyspec;

  if(self->parent == NULL)
    return;

  XUngrabKeyboard(xc->display, CurrentTime);
  XGetInputFocus(xc->display, &focused, &focus_ret);

  if (xc->debug) {
    keyspec = key_to_str(self->parent->key);
      fprintf(stderr, "Sending synthetic KeyPressEvent (%s) to window id %d\n", 
	      keyspec, (int)focused);
    free(keyspec);
  }

  e.display = xc->display;
  e.subwindow = None;
  e.time = CurrentTime;
  e.same_screen = True;	   
  e.x = e.y = e.x_root = e.y_root = 1;
  e.window = focused;
  e.keycode = self->parent->key->keycode;
  e.state = self->parent->key->modifiers;
  e.type = KeyPress;
  
  XSendEvent(xc->display, e.window, True, KeyPressMask, (XEvent *)&e);
  XSync(xc->display, False);

  XGrabKeyboard(xc->display, DefaultRootWindow(xc->display),
		True, GrabModeAsync, GrabModeAsync, CurrentTime);
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
      
      // get keycode and keystr
      keycode = ((XKeyPressedEvent*)&event)->keycode;
      keystr = XKeysymToString(XKeycodeToKeysym(xc->display, keycode, 0));
      
      // check if this key is a modifier 
      if (xc_keycode_to_modmask(xc, keycode) != 0) {
	break;
      }      
      else {	
	// non-modifier key hit...
	key = key_new(keystr);
	key->modifiers = xc_get_modifiers(xc);
	
	for(i=0; i<self->parent->num_children; i++) {
	  binding = self->parent->children[i];

	  if ( strcmp(binding->action, ":repeat") == 0) { 
	    if ( key_equals(binding->key, key) ) {
	      binding_exec(binding);
	      abort = False;
	      break;
	    }
	  }
	  abort = True;
	}
	
	if(abort) {
	  /* if the aborting key matches any root chains or chroots,
	   * prepare xc->reentry */
	  for(i=0; i<xc->root->num_children; i++) {
	    binding = xc->root->children[i];
	    if ( strcmp(binding->action, ":enter") == 0 &&
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
    fprintf(stderr, "    ");  

  if(depth > 0) {
    keyspec = key_to_str(self->key);
    fprintf(stderr, "%s %s %s\n", keyspec, self->action, self->argument);
    free(keyspec);
  }

  for( i=0; i<self->num_children; i++ ) {
    binding_list(self->children[i]);
  }
}
