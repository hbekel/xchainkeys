#ifndef UTIL_H
#define UTIL_H

void usage(void);
void version(void);
unsigned int get_modifiers(Display *display);
unsigned int modname_to_modifier(char *str);
unsigned int keycode_to_modifier(XModifierKeymap *xmodmap, KeyCode keycode);
void send_key(Display *display, Key_t *key, Window window);
long get_msec(void);

#endif /* ifndef XCHAINKEYS_H */
