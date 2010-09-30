#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif /* _XOPEN_SOURCE */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <X11/Xlib.h>

#include "key.h"
#include "util.h"
#include "xchainkeys.h"

extern XChainKeys_t *xc;

Key_t* key_new(char *keyspec) {

  Key_t *self = (Key_t *) calloc(1, sizeof(Key_t));;
  
  if( key_parse_keyspec(self, keyspec) ) {
    return(self);
  }
  else {
    free(self);
    return(NULL);
  }
}

int key_parse_keyspec(Key_t *self, char *keyspec) {

  char str[256];
  char *original_keyspec = strdup(keyspec);
  int len;
  int ret = True;

  if (keyspec[0] == ':')
    return 0;

  /* parse modifiers */
  while( strstr(keyspec, "-") != NULL ) {

    len = strcspn(keyspec, "-");
    strncpy(str, keyspec, 256);
    str[len] = '\0';
    
    if(!key_add_modifier(self, str)) {
      free(original_keyspec);
      return False;
    }
    keyspec += len + 1;
  }
  
  /* keyspec now contains the keysym string */
  if(XStringToKeysym(keyspec) != NoSymbol) {
    self->keysym = XStringToKeysym(keyspec);
  }
  else {
    ret = False;
  }
  
  free(original_keyspec);
  return ret;
}

int key_add_modifier(Key_t *self, char *str) {
  unsigned int modifier = 0;
  
  if( (modifier = modname_to_modifier(str)) != 0 )
    self->modifiers |= modifier;
  else
    return False;
  
  return True;
}

int key_get_keycode(Key_t *self) {
  return XKeysymToKeycode(xc->display, self->keysym);
}

int key_equals(Key_t *self, Key_t *key) {
  if(self->modifiers == key->modifiers &&
     key_get_keycode(self) == key_get_keycode(key))
    return 1;
  return 0;
}

void key_grab(Key_t *self) {
  int i;

  for( i=0; i<8; i++ ) {
    XGrabKey(xc->display, key_get_keycode(self), self->modifiers | xc->modmask[i], 
	     DefaultRootWindow(xc->display), False,
	     GrabModeAsync, GrabModeAsync);
  }
}

void key_ungrab(Key_t *self) {
  int i;

  for( i=0; i<8; i++ ) {
    XUngrabKey(xc->display, key_get_keycode(self), self->modifiers | xc->modmask[i], 
	       DefaultRootWindow(xc->display));
  }
}

char *key_to_str(Key_t *self) {

  char *str = (char *) calloc(256, sizeof(char));

  if (self->modifiers & LockMask)    strcat(str, "lock-");
  if (self->modifiers & ControlMask) strcat(str, "C-");
  if (self->modifiers & Mod1Mask)    strcat(str, "A-");
  if (self->modifiers & Mod2Mask)    strcat(str, "mod2-");
  if (self->modifiers & Mod3Mask)    strcat(str, "mod3-");
  if (self->modifiers & Mod4Mask)    strcat(str, "W-");
  if (self->modifiers & Mod5Mask)    strcat(str, "mod5-");
  if (self->modifiers & ShiftMask)   strcat(str, "S-");

  strcat(str, XKeysymToString(self->keysym));

  return str;	 
}
