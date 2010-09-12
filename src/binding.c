#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <X11/Xlib.h>

#include "key.h"
#include "binding.h"
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

  if(strcmp(self->action, ":abort") == 0)
    binding_abort(self);

  if(strcmp(self->action, ":exec") == 0)
    binding_exec(self);

  free(path);
}

void binding_enter(Binding_t *self) {
  XEvent event;
  KeyCode keycode;
  Binding_t *binding;
  Key_t *key;
  char *keystr;
  int done = False;

  // get exclusive grab on keyboard...
  if (self->parent == xc->root) {
    XGrabKeyboard(xc->display, DefaultRootWindow(xc->display),
		  True, GrabModeAsync, GrabModeAsync, CurrentTime);

    if (xc->debug) fprintf(stderr, "Keyboard grabbed...\n");
  }

  while(!done) {
    // wait for events, timeout after XC_TIMEOUT msecs
    XNextEvent(xc->display, &event);

    // dispatch exec, abort or escape  
    switch(event.type) {
    case KeyPress:

      // get keycode and keystr
      keycode = ((XKeyPressedEvent*)&event)->keycode;
      keystr = XKeysymToString(XKeycodeToKeysym(xc->display, keycode, 0));

      // check if this key is a modifier 
      if (xc_keycode_to_modmask(xc, keycode) != 0) {
	if (xc->debug) fprintf(stderr, "Modifier pressed: %s\n", keystr);
	break;
      }      
      else {	
	// non-modifier key hit...
	key = key_new(keystr);
	key->modifiers = xc_get_modifiers(xc);
	
	if (xc->debug) fprintf(stderr, "Key pressed: %s\n", key_to_str(key));	
	
	// check if this key is bound in this keymap
	if( (binding = binding_get_child_by_key(self, key)) != NULL) {

	  // activate the binding
	  binding_activate(binding);
	}
	else {
	  if (xc->debug) fprintf(stderr, "No binding!\n");
	}	
	// done, exit this keymap
	done = True;
	break;
      }
      free(key);
    }
  }

  // ungrab keyboard...
  if (self->parent == xc->root) {
    XUngrabKeyboard(xc->display, CurrentTime);
    if (xc->debug) fprintf(stderr, "Keyboard ungrabbed...\n");
  }
}

void binding_abort(Binding_t *self) {
   if (xc->debug) fprintf(stderr, "Abort!\n");
}

void binding_escape(Binding_t *self) {

  XKeyEvent e;
  Window focused;
  int focus_ret;

  if(self->parent == NULL)
    return;

  XUngrabKeyboard(xc->display, CurrentTime);
  XGetInputFocus(xc->display, &focused, &focus_ret);
	    
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
}

void binding_exec(Binding_t *self) {
  pid_t pid;
  char *command = self->argument;

  if (xc->debug) fprintf (stderr, "Spawning %s\n", command);
  
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
  char *path = (char *) calloc(1, 4096*sizeof(char));
  char *keystr;
  do {
    keystr = key_to_str(binding->key);
    if(strlen(path) > 0)
      strncat(keystr, " ", 1);

    memmove(path+strlen(keystr), path, strlen(keystr));
    memmove(path, keystr, strlen(keystr));

    binding = binding->parent;
  } while(binding->parent != NULL);
  return path;
}

void binding_list(Binding_t *self) {
  Binding_t *current;
  int depth = 0;
  int i;
  
  current = self;
  while( current->parent != NULL ) {
    depth++;
    current = current->parent;      
  }
  for( i=1; i<depth; i++ )
    fprintf(stderr, "  ");

  if(depth > 0) 
    fprintf(stderr, "%s %s %s\n",
	    key_to_str(self->key),
	    self->action,
	    self->argument);

  for( i=0; i<self->num_children; i++ ) {
    binding_list(self->children[i]);
  }
}

  
