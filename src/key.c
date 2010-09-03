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
    fprintf(stderr, "Key: parse_keyspec: %#x %#x\n", self->modmask, self->keycode);

  return ret;
}

int key_add_modifier(Key_t *self, char *str) {
  
  if (xc->debug) fprintf(stderr, "Key: add_modifier %s\n", str);

  if( strcmp(str, "C") == 0 ) {
    self->modmask |= ControlMask;
    return 1;
  }
  if( strcmp(str, "A") == 0 ) {
    self->modmask |= Mod1Mask;
    return 1;
  }
  if( strcmp(str, "W") == 0) {
    self->modmask |= Mod4Mask;
    return 1;
  }
  if( strcmp(str, "S") == 0) {
    self->modmask |= ShiftMask;
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
  if(self->modmask == key->modmask &&
     self->keycode == key->keycode)
    return 1;
  return 0;
}

char *key_to_str(Key_t *self) {
  if (self == NULL)
    return "<none>";

  char *str = (char *) calloc(512, sizeof(char));

  if (self->modmask & ControlMask) strcat(str, "C-");
  if (self->modmask & ShiftMask)   strcat(str, "S-");
  if (self->modmask & Mod1Mask)    strcat(str, "A-");
  if (self->modmask & Mod4Mask)    strcat(str, "W-");

  strcat(str, XKeysymToString(XKeycodeToKeysym(xc->display, self->keycode, 0)));

  return str;	 
}
