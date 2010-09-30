#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <X11/Xlib.h>

#include "key.h"
#include "xchainkeys.h"

extern XChainKeys_t *xc;

void version() {
  printf("%s %s Copyright (C) 2010 Henning Bekel <%s>\n",
	 PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_BUGREPORT);

  printf("License GPLv3: GNU GPL version 3 <http://gnu.org/licenses/gpl.html>\n\n");
  printf("This is free software; you are free to change and redistribute it.\n");
  printf("There is NO WARRANTY, to the extent permitted by law.\n");
  fflush(stdout);
}

void usage() {
  version();
  printf("\n");
  printf("Usage: %s [options]\n\n", PACKAGE_NAME);
  printf("  -f, --file    : alternative config file\n");
  printf("  -k, --keys    : Show valid keyspecs\n");
  printf("  -d, --debug   : Enable debug messages\n");
  printf("  -h, --help    : Print this help text\n");
  printf("  -v, --version : Print version information\n");
  printf("\n");
  fflush(stdout);
}

unsigned int modname_to_modifier(char *str) {

  if( strcasecmp(str, "shift") == 0 || strcmp(str, "S") == 0)
    return ShiftMask;

  if( strcasecmp(str, "lock") == 0 )
    return LockMask;

  if( strcasecmp(str, "control") == 0 || strcmp(str, "C") == 0)
    return ControlMask;

  if( strcasecmp(str, "mod1") == 0 || strcmp(str, "A") == 0 || strcmp(str, "M") == 0 )
    return Mod1Mask;

  if( strcasecmp(str, "mod2") == 0 )
    return Mod2Mask;

  if( strcasecmp(str, "mod3") == 0 )
    return Mod3Mask;

  if( strcasecmp(str, "mod4") == 0 || strcmp(str, "W") == 0 || strcmp(str, "H") == 0 )
    return Mod4Mask;

  if( strcasecmp(str, "mod5") == 0 )
    return Mod5Mask;

  return 0;
}

unsigned int keycode_to_modifier(XModifierKeymap *xmodmap, KeyCode keycode) {

  int i = 0;
  int j = 0;
  int max = xmodmap->max_keypermod;

  for (i = 0; i < 8; i++) {
    for (j = 0; j < max && xmodmap->modifiermap[(i * max) + j]; j++) {
      if (keycode == xmodmap->modifiermap[(i * max) + j]) {
        switch (i) {
          case ShiftMapIndex: return ShiftMask; break;
          case LockMapIndex: return LockMask; break;
          case ControlMapIndex: return ControlMask; break;
          case Mod1MapIndex: return Mod1Mask; break;
          case Mod2MapIndex: return Mod2Mask; break;
          case Mod3MapIndex: return Mod3Mask; break;
          case Mod4MapIndex: return Mod4Mask; break;
          case Mod5MapIndex: return Mod5Mask; break;
        }
      } 
    } 
  } 
  return (KeyCode) 0;
}

unsigned int get_modifiers(Display *display) {

  char keymap[32]; 
  unsigned int keycode;
  unsigned int modifiers = 0;

  XQueryKeymap(display, keymap);

  for (keycode = 0; keycode < 256; keycode++) {
    if ((keymap[(keycode / 8)] & (1 << (keycode % 8))) \
        && keycode_to_modifier(xc->xmodmap, keycode)) {

      modifiers |= keycode_to_modifier(xc->xmodmap, keycode);
    }
  }
  return modifiers;
}

void send_key(Display *display, Key_t *key, Window window) {

  XKeyEvent e;
  char *keyspec;

  if (xc->debug) {
    keyspec = key_to_str(key);
      printf("Sending synthetic KeyPressEvent (%s) to window id %d\n", 
	      keyspec, (int)window);
      fflush(stdout);
    free(keyspec);
  }

  e.display = display;
  e.window = window;
  e.subwindow = None;
  e.time = CurrentTime;
  e.same_screen = True;	   
  e.keycode = key_get_keycode(key);
  e.state = key->modifiers;
  e.type = KeyPress;
  e.x = e.y = e.x_root = e.y_root = 1;
  
  XSendEvent(display, e.window, True, KeyPressMask, (XEvent *)&e);
  XSync(display, False);
}

long get_msec(void) {
  long msec;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  msec = (long)(tv.tv_sec*1000 + (tv.tv_usec/1000));
  return msec;
}
