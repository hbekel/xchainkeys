#ifndef XCHAINKEYS_H
#define XCHAINKEYS_H

typedef struct XChainKeys {
  Display *display;  
  XModifierKeymap *xmodmap;
  int modmask[8];
  char *action_names[6];
  int num_actions;
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

#define XC_ACTION_NONE   0
#define XC_ACTION_ENTER  1
#define XC_ACTION_ESCAPE 2
#define XC_ACTION_ABORT  3
#define XC_ACTION_EXEC   4
#define XC_ACTION_REPEAT 5

#define XC_ABORT_AUTO 1
#define XC_ABORT_MANUAL 0

#endif /* ifndef XCHAINKEYS_H */
