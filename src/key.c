#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>

#include "key.h"
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
  int len, ret;

  if (keyspec[0] == ':')
    return 0;

  /* parse modifiers */
  while( strstr(keyspec, "-") != NULL ) {

    len = strcspn(keyspec, "-");
    strncpy(str, keyspec, len);
    str[len] = '\0';
    
    key_add_modifier(self, str);

    keyspec += len + 1;
  }
  
  /* keyspec now contains the keysym string */
  ret = key_set_keycode(self, keyspec);
  
  if (xc->debug)
    fprintf(stderr, "Key: parse_keyspec: %#x %#x\n", self->modifiers, self->keycode);

  return ret;
}

int key_add_modifier(Key_t *self, char *str) {
  
  if (xc->debug) fprintf(stderr, "Key: add_modifier %s\n", str);

  if( strcmp(str, "shift") == 0 || strcmp(str, "S") == 0) {
    self->modifiers |= ShiftMask;
    return 1;
  }

  if( strcmp(str, "lock") == 0 || strcmp(str, "L") == 0) {
    self->modifiers |= LockMask;
    return 1;
  }

  if( strcmp(str, "control") == 0 || strcmp(str, "C") == 0) {
    self->modifiers |= ControlMask;
    return 1;
  }

  if( strcmp(str, "mod1") == 0 || strcmp(str, "A") == 0 ) {
    self->modifiers |= Mod1Mask;
    return 1;
  }
  if( strcmp(str, "mod2") == 0 || strcmp(str, "N") == 0 ) {
    self->modifiers |= Mod2Mask;
    return 1;
  }

  if( strcmp(str, "mod3") == 0 ) {
    self->modifiers |= Mod3Mask;
    return 1;
  }

  if( strcmp(str, "mod4") == 0 || strcmp(str, "W") == 0) {
    self->modifiers |= Mod4Mask;
    return 1;
  }

  if( strcmp(str, "mod5") == 0 || strcmp(str, "I") == 0) {
    self->modifiers |= Mod4Mask;
    return 1;
  }

  return 0;
}

int key_set_keycode(Key_t *self, char *str) {
  KeySym keysym;

  if (xc->debug)
    fprintf(stderr, "Key: set_keycode %s\n", str);

  keysym = XStringToKeysym(str);
  if( keysym == NoSymbol )
    return 0;

  self->keycode = XKeysymToKeycode(xc->display, keysym);
  return 1;
}

int key_equals(Key_t *self, Key_t *key) {
  if(self->modifiers == key->modifiers &&
     self->keycode == key->keycode)
    return 1;
  return 0;
}

void key_grab(Key_t *self) {
  XGrabKey(xc->display, self->keycode, self->modifiers, 
	   DefaultRootWindow(xc->display), 0,
	   GrabModeSync, GrabModeSync);
}

void key_ungrab(Key_t *self) {
  XUngrabKey(xc->display, self->keycode, self->modifiers, 
	   DefaultRootWindow(xc->display));
}

char *key_to_str(Key_t *self) {
  if (self == NULL)
    return "<none>";

  char *str = (char *) calloc(512, sizeof(char));

  if (self->modifiers & ShiftMask)   strcat(str, "S-");
  if (self->modifiers & ControlMask) strcat(str, "C-");
  if (self->modifiers & LockMask)   strcat(str, "L-");
  if (self->modifiers & Mod1Mask)    strcat(str, "A-");
  if (self->modifiers & Mod2Mask)    strcat(str, "N-");
  if (self->modifiers & Mod3Mask)    strcat(str, "mod3-");
  if (self->modifiers & Mod4Mask)    strcat(str, "W-");
  if (self->modifiers & Mod5Mask)    strcat(str, "I-");

  strcat(str, XKeysymToString(XKeycodeToKeysym(xc->display, self->keycode, 0)));

  return str;	 
}
