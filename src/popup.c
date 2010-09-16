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

void* popup_new(Display *display, char *font, char *fg, char *bg) {

  Popup_t *self = (Popup_t *) calloc(1, sizeof(Popup_t)); 
  self->display = display;
  self->enabled = True;

  XSetWindowAttributes winattrs;
  winattrs.override_redirect = True;

  self->window = XCreateWindow(self->display, 
			    DefaultRootWindow(self->display),
			    0, 0, 1, 1, 2, 
			    CopyFromParent, 
			    InputOutput, 
			    CopyFromParent, 
			    CWOverrideRedirect, 
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

  self->w = XTextWidth(self->font, self->text, strlen(self->text)) + margin*2;
  self->h = self->font->ascent + self->font->descent + margin*2;

  self->x = DisplayWidth(self->display, DefaultScreen(self->display)) / 2;
  self->x -= (self->w + margin) / 2;

  self->y = DisplayHeight(self->display, DefaultScreen(self->display)) / 2;
  self->y -= (self->h + margin) / 2;

  XMoveResizeWindow(self->display, self->window,
		    self->x, self->y, self->w, self->h);
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

void popup_destroy(Popup_t *self) {
  XFreeGC(self->display, self->gc);
  free(self);
}
