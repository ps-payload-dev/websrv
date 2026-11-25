/* Copyright (C) 2024 John Törnblom
   Copyright (C) 2026 ItsJokerZz

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

#include <arpa/inet.h>
#include <net/if.h>
#include <execinfo.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/syscall.h>

#include <ps5/kernel.h>

#include "fs.h"
#include "hbldr.h"
#include "http.h"
#include "notify.h"
#include "sys.h"
#include "websrv.h"
#include "log.h"
#include <elfldr.h>

#define INCASSET(name, file)                                                  \
  __asm__(".section .rodata\n"                                                \
          ".global " #name "\n"                                               \
          ".global " #name "_end\n"                                           \
          ".global " #name "_size\n"                                          \
          ".align 16\n" #name ":\n"                                           \
          ".incbin \"" file "\"\n" #name "_end:\n" #name "_size:\n"           \
          ".quad " #name "_end - " #name "\n"                                 \
          ".previous\n");                                                     \
  extern const uint8_t name[];                                                \
  extern const size_t name##_size;

INCASSET(param, "assets/pkg/param.json");
INCASSET(icon0, "assets/pkg/icon0.png");

typedef struct app_launch_ctx {
  uint32_t structsize;
  uint32_t user_id;
  uint32_t app_opt;
  uint64_t crash_report;
  uint32_t check_flag;
} app_launch_ctx_t;

int sceAppInstUtilInitialize(void);
int sceAppInstUtilAppInstallAll(void *);

int sceUserServiceInitialize(void *);
int sceUserServiceGetForegroundUser(uint32_t *user_id);
void sceUserServiceTerminate(void);

int sceSystemServiceGetAppIdOfRunningBigApp(void);
int sceSystemServiceKillApp(int app_id, int how, int reason, int core_dump);
int sceSystemServiceLaunchApp(const char *title_id, char **argv,
                              app_launch_ctx_t *ctx);

static int
install_app(const char *title_id, const char *dir) {
  int (*sceAppInstUtilAppInstallTitleDir)(const char *, const char *, void *)
      = 0;
  const char *nid = "Wudg3Xe3heE";
  uint32_t handle;

  if(!kernel_dynlib_handle(-1, "libSceAppInstUtil.sprx", &handle)) {
    sceAppInstUtilAppInstallTitleDir
        = (void *)kernel_dynlib_resolve(-1, handle, nid);
  }

  if(sceAppInstUtilAppInstallTitleDir) {
    return sceAppInstUtilAppInstallTitleDir(title_id, dir, 0);
  }

  return sceAppInstUtilAppInstallAll(0);
}

static int
install_file(const char *path, const uint8_t *data, size_t size) {
  FILE *f;

  if(!(f = fopen(path, "w"))) {
    return -1;
  }

  if(fwrite(data, size, 1, f) != 1) {
    fclose(f);
    return -1;
  }

  fclose(f);
  return 0;
}

static int
install_launcher(void) {
  struct stat st;

  if(!stat("/user/appmeta/FAKE00000/param.json", &st)) {
    return 0;
  }
  if(!stat("/user/app/FAKE00000/param.json", &st)) {
    return 0;
  }

  if(mkdir("/user/app/FAKE00000", 0755) && errno != EEXIST) {
    LOG_PERROR("mkdir");
    return -1;
  }
  if(mkdir("/user/app/FAKE00000/sce_sys", 0755) && errno != EEXIST) {
    LOG_PERROR("mkdir");
    return -1;
  }

  if(install_file("/user/app/FAKE00000/sce_sys/param.json", param,
                  param_size)) {
    return -1;
  }
  if(install_file("/user/app/FAKE00000/sce_sys/icon0.png", icon0,
                  icon0_size)) {
    return -1;
  }

  return install_app("FAKE00000", "/user/app/");
}

__attribute__((constructor)) static void
sys_init(void) {
  pid_t pid;
  int err;

  signal(SIGSEGV, on_fatal_signal);
  signal(SIGABRT, on_fatal_signal);
  signal(SIGFPE, on_fatal_signal);
  signal(SIGILL, on_fatal_signal);
  signal(SIGBUS, on_fatal_signal);
  signal(SIGTRAP, on_fatal_signal);
  signal(SIGSYS, on_fatal_signal);

  if((err = sceUserServiceInitialize(0))) {
    LOG_PERROR("sceUserServiceInitialize");
    exit(err);
  }

  syscall(SYS_thr_set_name, -1, "websrv.elf");

  while((pid = sys_find_pid("websrv.elf")) > 0) {
    if((err = kill(pid, SIGKILL))) {
      LOG_PERROR("kill");
      exit(err);
    }
    sleep(1);
  }

  kernel_set_ucred_authid(-1, 0x4801000000000013L);

  if(!sceAppInstUtilInitialize()) {
    install_launcher();
  }

  sys_notify_address("Serving HTTP on", 8080);
}

__attribute__((destructor)) static void
sys_fini(void) {
  sceUserServiceTerminate();
}
