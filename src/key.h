#ifndef KEY_H
#define KEY_H

typedef struct Key {
  unsigned int modifiers;
  KeySym keysym;
  unsigned int keycode;
} Key_t;

Key_t* key_new(char *keyspec);
int key_parse_keyspec(Key_t *key, char *keyspec);
int key_add_modifier(Key_t *self, char *str);
int key_get_keycode(Key_t *self);
int key_equals(Key_t *self, Key_t *key);
void key_grab(Key_t *self);
void key_ungrab(Key_t *self);
char *key_to_str(Key_t *self);

#endif /* #ifndef KEY_H */
