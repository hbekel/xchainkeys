#ifndef XCHAINKEYS_H
#define XCHAINKEYS_H

#define XC_ACTION_NONE     0
#define XC_ACTION_ENTER    1
#define XC_ACTION_ESCAPE   2
#define XC_ACTION_ABORT    3
#define XC_ACTION_EXEC     4
#define XC_ACTION_GROUP    5
#define XC_ACTION_LOAD     6
#define XC_NUM_ACTIONS     7

#define XC_ABORT_AUTO 1
#define XC_ABORT_MANUAL 0

typedef struct XChainKeys {
  Display *display;  
  XModifierKeymap *xmodmap;
  int modmask[8];
  char *action_names[XC_NUM_ACTIONS];
  int debug;
  unsigned int timeout;
  unsigned int delay;
  char *config;
  int reload;
  struct Popup *popup;
  struct Binding *root;
  struct Binding *reentry;
} XChainKeys_t;

XChainKeys_t* xc_new(void);
void xc_parse_options(XChainKeys_t *self, int argc, char **argv);
void xc_init_modmask(XChainKeys_t *self);
int xc_handle_error(Display *display, XErrorEvent *event);
void xc_show_keys(XChainKeys_t *self);
void xc_find_config(XChainKeys_t *self);
void xc_parse_config(XChainKeys_t *self);
void xc_grab_prefix_keys(XChainKeys_t *self);
void xc_mainloop(XChainKeys_t *self);
void xc_reload(XChainKeys_t *self);
void xc_reset(XChainKeys_t *self);
void xc_shutdown(XChainKeys_t *self);

#endif /* ifndef XCHAINKEYS_H */
