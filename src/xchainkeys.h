#ifndef XCHAINKEYS_H
#define XCHAINKEYS_H

typedef struct XChainKeys {
  Display *display;  
  int debug;
} XChainKeys_t;

void* xc_create(Display *display);
void xc_version(XChainKeys_t *self);
void xc_usage(XChainKeys_t *self);
void xc_mainloop(XChainKeys_t *self);
void xc_destroy(XChainKeys_t *self);

#endif /* ifndef XCHAINKEYS_H */
