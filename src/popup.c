#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif /* _XOPEN_SOURCE */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include "popup.h"

void* popup_new(Display *display, char *font, char *fg, char *bg, char *position) {

  Popup_t *self = (Popup_t *) calloc(1, sizeof(Popup_t)); 
  self->display = display;
  self->root = DefaultRootWindow(self->display);
  self->position = position;
  self->enabled = True;

  XSetWindowAttributes winattrs;
  winattrs.override_redirect = True;
  winattrs.cursor = popup_get_cursor(self);

  self->window = XCreateWindow(self->display, 
			    self->root,
			    0, 0, 1, 1, 2, 
			    CopyFromParent, 
			    InputOutput, 
			    CopyFromParent, 
			    CWOverrideRedirect | CWCursor,
			    &winattrs); 
  
  Colormap colormap = DefaultColormap(self->display, 0);
  XColor fgcolor;
  XColor bgcolor;

  XParseColor(self->display, colormap, fg, &fgcolor);
  XAllocColor(self->display, colormap, &fgcolor);

  XParseColor(self->display, colormap, bg, &bgcolor);
  XAllocColor(self->display, colormap, &bgcolor);

  XSetWindowBorder(self->display, self->window, fgcolor.pixel);
  XSetWindowBackground(self->display, self->window, bgcolor.pixel);

  XGCValues values;
  values.cap_style = CapButt;
  values.join_style = JoinBevel;
  values.foreground = fgcolor.pixel;
  values.background = bgcolor.pixel;

  unsigned long valuemask = GCCapStyle | GCJoinStyle;
  
  self->gc = XCreateGC(self->display, self->window, valuemask, &values);
  if (self->gc < 0) {
    printf("XCreateGC: \n");
    exit(EXIT_FAILURE);
  }
  XSetForeground(self->display, self->gc, fgcolor.pixel);

  self->font = XLoadQueryFont(self->display, font);
  if (!self->font) {
    fprintf(stderr, "%s: error: XLoadQueryFont: failed to load font '%s'\n", 
	    PACKAGE_NAME, font);
    exit(EXIT_FAILURE);
  }
  XSetFont(self->display, self->gc, self->font->fid);

  self->timeout = 0;
  self->mapped = False;
  self->w = self->h = 1;

  return self;
}

void popup_update(Popup_t *self) {
  
  int margin = 3;
  int dw = DisplayWidth(self->display, DefaultScreen(self->display));
  int dh = DisplayHeight(self->display, DefaultScreen(self->display));

  int parsed = 0;
  int centered_at_position = True;  
  char *ws = " \t";

  int x, y = 0;
  unsigned int ignored_width, ignored_height;
  Window root, child;
  unsigned int mask;

  self->x = 0;
  self->y = 0;

  self->w = XTextWidth(self->font, self->text, strlen(self->text)) + margin*2;
  self->h = self->font->ascent + self->font->descent + margin*2;

  if( (parsed = XParseGeometry(self->position, &x, &y,
			       &ignored_width, &ignored_height)) & (XValue | YValue)) {
    centered_at_position = False;

    self->x = (parsed & XNegative) ? dw - self->w - margin + x - 1 : x;
    self->y = (parsed & YNegative) ? dh - self->h - margin + y - 1 : y;
  }
  else if (strncmp(self->position, "center", 6) == 0) {
    self->x = dw / 2;
    self->y = dh / 2;
  }
  else if (strncmp(self->position, "mouse", 5) == 0) {

    XQueryPointer(self->display, self->root, &root, &child,
		  &self->x, &self->y, &x, &y, &mask);
  }
  else if(strlen(self->position) > 0) {
    self->x = atoi(self->position);
    self->y = atoi(self->position + strcspn(self->position, ws));
    centered_at_position = False;
  }
  
  if (centered_at_position) {
    self->x -= (self->w + margin) / 2;
    self->y -= (self->h + margin) / 2;
  }

  if (self->x < 0)
    self->x = 0;
  if (self->x > dw - (self->w + margin))
    self->x = dw - self->w - margin;
  if (self->y < 0)
    self->y = 0;
  if (self->y > dh - (self->h + margin))
    self->y = dh - self->h - margin;

  XMoveResizeWindow(self->display, self->window,
		    self->x, self->y, self->w, self->h);

  XClearArea(self->display, self->window,
	     0, 0, 0, 0,False);
  XFlush(self->display);

  XDrawString(self->display, self->window, self->gc,
	      margin, self->h - margin*2, 
	      self->text, strlen(self->text));

  XFlush(self->display);
}

void popup_show(Popup_t *self) {
  if(self->enabled) {
    XMapWindow(self->display, self->window);
    XRaiseWindow(self->display, self->window);
    self->mapped = True;
    popup_update(self);
  }
}

void popup_hide(Popup_t *self) {
  XUnmapWindow(self->display, self->window);
  self->mapped = False;
  XFlush(self->display);
}

void popup_set_timeout(Popup_t *self, unsigned int ms) {
  struct timeval tval;
  unsigned int now;

  gettimeofday(&tval, NULL);
  now =(long)(tval.tv_sec*1000 + (tval.tv_usec/1000));
  self->timeout = now + ms;
}

Cursor popup_get_cursor(Popup_t *self) {

  Pixmap mask;
  GC gc;
  XGCValues values;
  XColor color;
  Cursor cursor;

  mask = XCreatePixmap(self->display, self->root, 1, 1, 1);
  values.function = GXclear;
  gc = XCreateGC(self->display, mask, GCFunction, &values);

  XFillRectangle(self->display, mask, gc, 0, 0, 1, 1);
  color.pixel = 0; color.red = 0; color.flags = 04;

  cursor = XCreatePixmapCursor(self->display, mask, mask,
			       &color,&color, 0,0);

  XFreePixmap(self->display, mask);
  XFreeGC(self->display, gc);
  return cursor;
}

void popup_free(Popup_t *self) {
  XFreeFont(self->display, self->font);
  XFreeGC(self->display, self->gc);
  free(self);
}
