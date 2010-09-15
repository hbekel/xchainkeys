#ifndef XCHAINKEYS_H
#define XCHAINKEYS_H

typedef struct XChainKeys {
  Display *display;  
  XModifierKeymap *xmodmap;
  int modmask[8];
  int debug;
  unsigned int timeout;
  unsigned int delay;
  char *config;
  struct Popup *popup;
  struct Binding *root;
  struct Binding *reentry;
} XChainKeys_t;

XChainKeys_t* xc_new(Display *display);
void xc_version(XChainKeys_t *self);
void xc_usage(XChainKeys_t *self);
void xc_find_config(XChainKeys_t *self);
void xc_parse_config(XChainKeys_t *self);
void xc_mainloop(XChainKeys_t *self);
int xc_keycode_to_modmask(XChainKeys_t *self, KeyCode keycode);
unsigned int xc_get_modifiers(XChainKeys_t *self);
long xc_get_msec();

#endif /* ifndef XCHAINKEYS_H */
