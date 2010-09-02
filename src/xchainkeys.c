/* xchainkeys 0.1 -- exclusive keychains for X11
 * Copyright (C) 2010 Henning Bekel <h.bekel@googlemail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <X11/Xlib.h>

#include "xchainkeys.h"

XChainKeys_t *xc;

void* xc_create(Display *display) {
  XChainKeys_t *self = (XChainKeys_t *) calloc(1, sizeof(XChainKeys_t)); 
  self->display = display;
  self->debug = False;
  return self;
}

void xc_version(XChainKeys_t *self) {
  printf("xchainkeys %s Copyright (C) 2010 Henning Bekel <%s>\n",
	 PACKAGE_VERSION, PACKAGE_BUGREPORT);
}

void xc_usage(XChainKeys_t *self) {
  printf("Usage: xchainkeys [options]\n\n");
  printf("  -d, --debug   : Enable debug messages\n");
  printf("  -h, --help    : Print this help text\n");
  printf("  -v, --version : Print version information\n");
  printf("\n");
}

void xc_mainloop(XChainKeys_t *self) {
  printf("xc_mainloop: not implemented\n"); 
}

void xc_destroy(XChainKeys_t *self) {
  free(self);
}

int main(int argc, char **argv) {

  Display *display;

  struct option options[] = {
    { "debug", no_argument, NULL, 'd' },
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'v' },
    { 0, 0, 0, 0 },
  };
  int option, option_index;

  /* try to open the x display */
  if (NULL==(display=XOpenDisplay(NULL))) {
    perror(argv[0]);
    exit(EXIT_FAILURE);
  }

  /* create XChainkeys instance */
  xc = xc_create(display);
 
  /* parse command line arguments */
  while (1) {

    option = getopt_long(argc, argv, "dhv", options, &option_index);
    
    switch (option) {

    case 'd':
      xc->debug = True;
      break;
      
    case 'h':
      xc_usage(xc);
      exit(EXIT_SUCCESS);
      break;
      
    case 'v':
      xc_version(xc);
      exit(EXIT_SUCCESS);
      break;      

    case -1:
      break;

    default:
      xc_usage(xc);
      exit(EXIT_FAILURE);
      break;
    }
    if(option == -1)
      break;
  }

  /* enter mainloop */
  xc_mainloop(xc);

  exit(EXIT_SUCCESS);
}
