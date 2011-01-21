#ifndef BINDING_H
#define BINDING_H

struct Binding {
  Key_t *key;
  int action;
  char *argument;
  char *name;
  int timeout;
  int abort;
  struct Binding *parent;  
  int num_children;
  struct Binding *children[1024];
};
typedef struct Binding Binding_t;

Binding_t* binding_new();
void binding_set_action(Binding_t *self, char *str);
void binding_parse_arguments(Binding_t *self);
void binding_create_default_bindings(Binding_t *self);
void binding_append_child(Binding_t *self, Binding_t *child);
Binding_t *binding_get_child_by_key(Binding_t *self, Key_t *key);
Binding_t *binding_get_child_by_action(Binding_t *self, int action);
int binding_wait_event(Binding_t *self);
void binding_activate(Binding_t *self);
void binding_enter(Binding_t *self);
void binding_escape(Binding_t *self);
void binding_send(Binding_t *self);
void binding_exec(Binding_t *self);
void binding_group(Binding_t *self);
char *binding_to_path(Binding_t *self);
void binding_list(Binding_t *self);
void binding_free(Binding_t *self);

#endif /* #ifndef BINDING_H */
