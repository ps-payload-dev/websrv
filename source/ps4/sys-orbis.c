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

#include <sys/types.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <execinfo.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

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
#include <sys/mount.h>
#include <sys/uio.h>

#include <ps4/kernel.h>
#include <ps4/klog.h>

#include "fs.h"
#include "hbldr.h"
#include "http.h"
#include "notify.h"
#include "sys.h"
#include "websrv.h"
#include "log.h"
#include <elfldr.h>

#ifndef LAUNCHER_EBOOT
#define LAUNCHER_EBOOT "/dev/null"
#endif
#ifndef LAUNCHER_SELF
#define LAUNCHER_SELF "/dev/null"
#endif
#ifndef LAUNCHER_OPENER
#define LAUNCHER_OPENER "/dev/null"
#endif
#ifndef LAUNCHER_PARAM
#define LAUNCHER_PARAM "/dev/null"
#endif
#ifndef LAUNCHER_ICON0
#define LAUNCHER_ICON0 "/dev/null"
#endif

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

INCASSET(launcher_eboot, LAUNCHER_EBOOT);
INCASSET(launcher_self, LAUNCHER_SELF);
INCASSET(launcher_opener, LAUNCHER_OPENER);
INCASSET(launcher_param, LAUNCHER_PARAM);
INCASSET(launcher_icon0, LAUNCHER_ICON0);

#define IOVEC_ENTRY(x)                                                        \
  { x ? x : 0, x ? strlen(x) + 1 : 0 }
#define IOVEC_SIZE(x) (sizeof(x) / sizeof(struct iovec))

typedef struct app_launch_ctx {
  uint32_t structsize;
  uint32_t user_id;
  uint32_t app_opt;
  uint64_t crash_report;
  uint32_t check_flag;
} app_launch_ctx_t;

int sceAppInstUtilInitialize(void);
int sceAppInstUtilAppInstallTitleDir(const char *title_id, const char *dir,
                                     void *unused);
int sceAppInstUtilAppUnInstall(const char *title_id);

int sceUserServiceInitialize(void *);
int sceUserServiceGetForegroundUser(uint32_t *user_id);
void sceUserServiceTerminate(void);

int sceSystemServiceGetAppIdOfBigApp(void);
int sceSystemServiceKillApp(int app_id, int how, int reason, int core_dump);
int sceSystemServiceLaunchApp(const char *title_id, char **argv,
                              app_launch_ctx_t *ctx);

static int
remount_system_ex(void) {
  struct iovec iov[] = {
    IOVEC_ENTRY("from"),      IOVEC_ENTRY("/dev/da0x5.crypt"),
    IOVEC_ENTRY("fspath"),    IOVEC_ENTRY("/system_ex"),
    IOVEC_ENTRY("fstype"),    IOVEC_ENTRY("exfatfs"),
    IOVEC_ENTRY("large"),     IOVEC_ENTRY("yes"),
    IOVEC_ENTRY("timezone"),  IOVEC_ENTRY("static"),
    IOVEC_ENTRY("async"),     IOVEC_ENTRY(NULL),
    IOVEC_ENTRY("ignoreacl"), IOVEC_ENTRY(NULL),
  };

  return nmount(iov, IOVEC_SIZE(iov), MNT_UPDATE);
}

static int
make_dir(const char *path) {
  if(mkdir(path, 0777) && errno != EEXIST) {
    LOG_PERROR(path);
    return -1;
  }

  return 0;
}

static int
put_file(const char *path, const uint8_t *data, size_t size) {
  int fd;

  if((fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0777)) < 0) {
    LOG_PERROR(path);
    return -1;
  }

  if(size && write(fd, data, size) != (ssize_t)size) {
    LOG_PERROR(path);
    close(fd);
    return -1;
  }

  fchmod(fd, 0777);
  close(fd);

  return 0;
}

static int
file_matches(const char *path, const uint8_t *data, size_t size) {
  uint8_t buf[512];
  size_t off = 0;
  ssize_t n;
  int fd;
  int match = 1;

  if((fd = open(path, O_RDONLY)) < 0) {
    return 0;
  }

  while((n = read(fd, buf, sizeof(buf))) > 0) {
    if(off + n > size || memcmp(buf, data + off, n)) {
      match = 0;
      break;
    }
    off += n;
  }

  close(fd);
  return match && n >= 0 && off == size;
}

static int
sync_file(const char *path, const uint8_t *data, size_t size) {
  if(file_matches(path, data, size)) {
    return 0;
  }

  return put_file(path, data, size);
}

static int
install_launcher(void) {
  struct stat st;
  int installed;
  int initialized = 0;
  int err;

  installed = !stat("/user/app/FAKE00000/sce_sys/param.sfo", &st);

  umask(0);

  if(remount_system_ex()) {
    LOG_PERROR("remount /system_ex");
    return -1;
  }

  if(installed
     && !file_matches("/user/app/FAKE00000/sce_sys/param.sfo", launcher_param,
                      launcher_param_size)) {
    if((err = sceAppInstUtilInitialize())) {
      printf("sceAppInstUtilInitialize: 0x%x\n", err);
      return -1;
    }
    sceAppInstUtilAppUnInstall("FAKE00000");
    initialized = 1;
    installed = 0;
  }

  if(make_dir("/system_ex/app/FAKE00000")
     || make_dir("/system_ex/app/FAKE00000/sce_sys")
     || make_dir("/user/app/FAKE00000")
     || make_dir("/user/app/FAKE00000/sce_sys")) {
    return -1;
  }

  if(sync_file("/system_ex/app/FAKE00000/eboot.bin", launcher_eboot,
               launcher_eboot_size)
     || sync_file("/system_ex/app/FAKE00000/open.elf", launcher_opener,
                  launcher_opener_size)
     || (launcher_self_size
         && sync_file("/system_ex/app/FAKE00000/websrv.elf", launcher_self,
                      launcher_self_size))) {
    puts("install_launcher: failed to write app files");
    return -1;
  }

  if(installed) {
    return 0;
  }

  if(put_file("/user/app/FAKE00000/sce_sys/param.sfo", launcher_param,
              launcher_param_size)
     || put_file("/user/app/FAKE00000/sce_sys/icon0.png", launcher_icon0,
                 launcher_icon0_size)) {
    puts("install_launcher: failed to write app files");
    return -1;
  }

  if(!initialized && (err = sceAppInstUtilInitialize())) {
    printf("sceAppInstUtilInitialize: 0x%x\n", err);
    return -1;
  }

  if((err = sceAppInstUtilAppInstallTitleDir("FAKE00000", "/user/app/", 0))) {
    printf("sceAppInstUtilAppInstallTitleDir: 0x%x\n", err);
    return -1;
  }

  return put_file("/system_ex/app/FAKE00000/sce_sys/param.sfo", launcher_param,
                  launcher_param_size);
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

  kernel_set_ucred_authid(-1, 0x3801000000000013L);

  install_launcher();

  sys_notify_address("Serving HTTP on", 8080);
}

__attribute__((destructor)) static void
sys_fini(void) {
  sceUserServiceTerminate();
}
