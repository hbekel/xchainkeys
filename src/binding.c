#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>

#include "key.h"
#include "binding.h"
#include "xchainkeys.h"

Binding_t* binding_new() {
  Binding_t *self = (Binding_t *) calloc(1, sizeof(Binding_t));
  self->key = NULL;
  self->action = "";
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

void binding_list(Binding_t *self) {
  Binding_t *current;
  int depth = 0;
  int i;
  
  current = self;
  while( current->parent != NULL ) {
    depth++;
    current = current->parent;      
  }
  for( i=0; i<depth; i++ )
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

  
