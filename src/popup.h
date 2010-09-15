#ifndef POPUP_H
#define POPUP_H

typedef struct Popup {
  Display *display;
  Window window;
  XFontStruct *font;
  GC gc;
  int x, y, w, h;
  char text[4096];
  unsigned int timeout;
  int mapped;
  int enabled;
} Popup_t;

void* popup_new(Display *display, char *font, char *fg, char *bg);
void popup_update(Popup_t *self);
void popup_show(Popup_t *self);
void popup_hide(Popup_t *self);
void popup_set_timeout(Popup_t *self, unsigned int ms);
void popup_destroy(Popup_t *self);

#endif
