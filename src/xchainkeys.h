#ifndef XCHAINKEYS_H
#define XCHAINKEYS_H

#define XC_POPUP_DELAY 1000
#define XC_TIMEOUT 3000

typedef struct XChainKeys {
  Display *display;  
  XModifierKeymap *xmodmap;
  int modmask[8];
  int debug;
  struct Popup *popup;
  struct Binding *root;
} XChainKeys_t;

XChainKeys_t* xc_new(Display *display);
void xc_version(XChainKeys_t *self);
void xc_usage(XChainKeys_t *self);
void xc_parse_config(XChainKeys_t *self);
void xc_mainloop(XChainKeys_t *self);
int xc_keycode_to_modmask(XChainKeys_t *self, KeyCode keycode);
unsigned int xc_get_modifiers(XChainKeys_t *self);
long xc_get_msec();

#endif /* ifndef XCHAINKEYS_H */
