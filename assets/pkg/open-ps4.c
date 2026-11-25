/* Copyright (C) 2026 ItsJokerZz

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING. If not, see
<http://www.gnu.org/licenses/>.  */

#include "log.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define URL "http://127.0.0.1:8080/"

typedef struct shellui_launch_param {
  uint32_t size;
  int32_t user_id;
} shellui_launch_param_t;

int sceUserServiceInitialize(void *);
int sceUserServiceTerminate(void);

int sceSystemServiceLaunchWebBrowser(const char *uri, void *);

int sceKernelLoadStartModule(const char *path, unsigned long args,
                             const void *argp, unsigned int flags, void *opt,
                             int *res);
int sceKernelDlsym(int handle, const char *symbol, void **addr);

static int
launch_by_shellui(const char *uri) {
  int (*initialize)(void) = 0;
  int (*launch)(const char *, shellui_launch_param_t *) = 0;
  shellui_launch_param_t param;
  int handle;
  int err;

  if((handle
      = sceKernelLoadStartModule("libSceShellUIUtil.sprx", 0, 0, 0, 0, 0))
     < 0) {
    return -1;
  }

  sceKernelDlsym(handle, "sceShellUIUtilInitialize", (void **)&initialize);
  sceKernelDlsym(handle, "sceShellUIUtilLaunchByUri", (void **)&launch);

  if(!initialize || !launch) {
    return -1;
  }

  initialize();

  param.size = sizeof(param);
  param.user_id = -1;

  if((err = launch(uri, &param))) {
    return err;
  }

  return 0;
}

__attribute__((constructor)) static void
constructor(void) {
  if(sceUserServiceInitialize(0)) {
    LOG_PERROR("sceUserServiceInitialize");
  }
}

int
main(int argc, char *argv[]) {
  if(!launch_by_shellui(URL)) {
    return EXIT_SUCCESS;
  }

  if(sceSystemServiceLaunchWebBrowser(URL, 0)) {
    LOG_PERROR("sceSystemServiceLaunchWebBrowser");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

__attribute__((destructor)) static void
destructor(void) {
  sceUserServiceTerminate();
}
