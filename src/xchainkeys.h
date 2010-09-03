#ifndef XCHAINKEYS_H
#define XCHAINKEYS_H

#define debug(a) fprintf(stderr, a)

typedef struct XChainKeys {
  Display *display;  
  int debug;
  struct Binding *root;
} XChainKeys_t;

XChainKeys_t* xc_new(Display *display);
void xc_version(XChainKeys_t *self);
void xc_usage(XChainKeys_t *self);
void xc_parse_config(XChainKeys_t *self, char *path);
void xc_mainloop(XChainKeys_t *self);
void xc_destroy(XChainKeys_t *self);

#endif /* ifndef XCHAINKEYS_H */
