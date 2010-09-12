#ifndef BINDING_H
#define BINDING_H

struct Binding {
  Key_t *key;
  char *action;
  char *argument;
  struct Binding *parent;
  
  int num_children;
  struct Binding *children[1024];
};
typedef struct Binding Binding_t;

Binding_t* binding_new();
void binding_append_child(Binding_t *self, Binding_t *child);
Binding_t *binding_get_child_by_key(Binding_t *self, Key_t *key);
Binding_t *binding_get_child_by_action(Binding_t *self, char *action);
void binding_activate(Binding_t *self);
void binding_enter(Binding_t *self);
void binding_escape(Binding_t *self);
void binding_abort(Binding_t *self);
void binding_exec(Binding_t *self);
char *binding_to_path(Binding_t *self);
void binding_list(Binding_t *self);

#endif /* #ifndef BINDING_H */
