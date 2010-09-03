#ifndef KEY_H
#define KEY_H

typedef struct Key {
  unsigned int modifiers;
  KeyCode keycode;
} Key_t;

Key_t* key_new(char *keyspec);
int key_parse_keyspec(Key_t *key, char *keyspec);
int key_add_modifier(Key_t *self, char *str);
int key_set_keycode(Key_t *self, char *str);
int key_equals(Key_t *self, Key_t *key);
char *key_to_str(Key_t *self);

#endif /* #ifndef KEY_H */
