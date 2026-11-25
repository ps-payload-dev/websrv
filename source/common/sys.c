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
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>

#if defined(__ORBIS__) || defined(__PROSPERO__)
#define SYS_CONSOLE 1
#include <execinfo.h>
#include <sys/sysctl.h>
#include <elfldr.h>
#include "http.h"
#include "notify.h"
#endif

#include "fs.h"
#include "hbldr.h"
#include "log.h"
#include "sys.h"

#ifdef SYS_CONSOLE
typedef struct app_launch_ctx {
  uint32_t structsize;
  uint32_t user_id;
  uint32_t app_opt;
  uint64_t crash_report;
  uint32_t check_flag;
} app_launch_ctx_t;

int sceUserServiceGetForegroundUser(uint32_t *user_id);

int sceSystemServiceKillApp(int app_id, int how, int reason, int core_dump);
int sceSystemServiceLaunchApp(const char *title_id, char **argv,
                              app_launch_ctx_t *ctx);

#ifdef __PROSPERO__
int sceSystemServiceGetAppIdOfRunningBigApp(void);
#define BIGAPP_ID() sceSystemServiceGetAppIdOfRunningBigApp()
#else
int sceSystemServiceGetAppIdOfBigApp(void);
#define BIGAPP_ID() sceSystemServiceGetAppIdOfBigApp()
#endif
#endif

static char *
args_decode(const char *s) {
  size_t length = strlen(s);
  char *arg = malloc(length + 1);
  size_t off = 0;
  int escape = 0;

  for(size_t i = 0; i < length; i++) {
    if(s[i] == '\\' && !escape) {
      escape = 1;
    } else {
      arg[off++] = s[i];
      escape = 0;
    }
  }

  arg[off] = 0;
  return arg;
}

static int
args_split(const char *args, char **argv, size_t size) {
  const char *p = args ? args : "";
  int argc = 0;

  if(!size) {
    return 0;
  }

  for(size_t i = 0; i < size; i++) {
    argv[i] = NULL;
  }

  while(argc < (int)size - 1) {
    const char *start;
    char *token;
    size_t length;

    while(*p == ' ') {
      p++;
    }
    if(!*p) {
      break;
    }

    start = p;
    while(*p && *p != ' ') {
      p++;
    }

    length = p - start;
    if(!(token = malloc(length + 1))) {
      break;
    }
    memcpy(token, start, length);
    token[length] = 0;

    argv[argc] = args_decode(token);
    free(token);
    if(!argv[argc]) {
      break;
    }
    argc++;
  }
  argv[argc] = NULL;

  return argc;
}

static void
args_free(char **argv) {
  for(int i = 0; argv[i]; i++) {
    free(argv[i]);
  }
}

#ifdef SYS_CONSOLE
pid_t
sys_find_pid(const char *name) {
  int mib[4] = { 1, 14, 8, 0 };
  pid_t mypid = getpid();
  pid_t pid = -1;
  size_t buf_size;
  uint8_t *buf;

  if(sysctl(mib, 4, 0, &buf_size, 0, 0)) {
    LOG_PERROR("sysctl");
    return -1;
  }

  if(!(buf = malloc(buf_size))) {
    LOG_PERROR("malloc");
    return -1;
  }

  if(sysctl(mib, 4, buf, &buf_size, 0, 0)) {
    LOG_PERROR("sysctl");
    free(buf);
    return -1;
  }

  for(uint8_t *ptr = buf; ptr < (buf + buf_size);) {
    int ki_structsize = *(int *)ptr;
    pid_t ki_pid = *(pid_t *)&ptr[72];
    char *ki_tdname = (char *)&ptr[447];

    ptr += ki_structsize;
    if(!strcmp(name, ki_tdname) && ki_pid != mypid) {
      pid = ki_pid;
    }
  }

  free(buf);

  return pid;
}

pid_t
sys_spawn(const char *cwd, int stdio, char *const argv[], char *const envp[],
          const uint8_t *elf, size_t elf_size) {
  return elfldr_spawn(cwd, stdio, argv, envp, elf, elf_size);
}
#endif

int
sys_launch_homebrew(const char *cwd, const char *path, const char *args,
                    const char *env) {
  char *argv[255];
  char *envp[255];
  int fds[2];
  pid_t pid;

  if(!cwd) {
    cwd = "/";
  }

  if(!args) {
    args = "";
  }

  if(!env) {
    env = "";
  }

  LOG_PRINTF("launch homebrew: CWD=%s %s %s %s\n", cwd, env, path, args);

  if(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
    LOG_PERROR("socketpair");
    return -1;
  }

#ifdef SO_NOSIGPIPE
  if(setsockopt(fds[1], SOL_SOCKET, SO_NOSIGPIPE, &(int){ 1 }, sizeof(int))
     < 0) {
    LOG_PERROR("setsockopt");
    close(fds[0]);
    close(fds[1]);
    return -1;
  }
#endif

  args_split(args, argv, 255);
  args_split(env, envp, 255);
  pid = hbldr_launch(cwd, path, fds[1], argv, envp);
  LOG_PRINTF("launch homebrew: hbldr_launch returned pid=%d errno=%d\n", pid,
             errno);

  args_free(argv);
  args_free(envp);

  close(fds[1]);
  if(pid < 0) {
    close(fds[0]);
    return -1;
  }

  return fds[0];
}

int
sys_launch_daemon(const char *cwd, const char *uri, const char *args,
                  const char *env) {
  uint8_t *elf = 0;
  char *argv[255];
  char *envp[255];
  int fds[2];
  pid_t pid;
  size_t elf_size = 0;

  if(!cwd) {
    cwd = "/";
  }
  if(!args) {
    args = "";
  }
  if(!env) {
    env = "";
  }

  if(uri[0] == '/') {
    elf = fs_readfile(uri, &elf_size);
  } else if(!strncmp(uri, "file:", 5)) {
    elf = fs_readfile(uri + 5, &elf_size);
#ifdef SYS_CONSOLE
  } else if(!strncmp(uri, "http:", 5) || !strncmp(uri, "https:", 6)) {
    elf = http_get(uri, &elf_size);
#endif
  }

  if(!elf) {
    return -1;
  }

  LOG_PRINTF("launch daemon: CWD=%s %s %s %s\n", cwd, env, uri, args);

  if(pipe(fds) == -1) {
    LOG_PERROR("pipe");
    free(elf);
    return -1;
  }

  args_split(args, argv, 255);
  args_split(env, envp, 255);
  pid = sys_spawn(cwd, fds[1], argv, envp, elf, elf_size);

  free(elf);
  args_free(argv);
  args_free(envp);

  close(fds[1]);
  if(pid < 0) {
    close(fds[0]);
    return -1;
  }

  return fds[0];
}

int
sys_launch_payload(const char *cwd, uint8_t *elf, size_t elf_size,
                   const char *args, const char *env) {
  char *argv[255];
  char *envp[255];
  int fds[2];
  pid_t pid;

  if(!cwd) {
    cwd = "/";
  }

  if(!args) {
    args = "";
  }

  if(!env) {
    env = "";
  }

  LOG_PRINTF("launch payload: CWD=%s %s %s\n", cwd, env, args);

  if(pipe(fds) == -1) {
    LOG_PERROR("pipe");
    return -1;
  }

  args_split(args, argv, 255);
  args_split(env, envp, 255);
  pid = sys_spawn(cwd, fds[1], argv, envp, elf, elf_size);

  args_free(argv);
  args_free(envp);

  close(fds[1]);
  if(pid < 0) {
    close(fds[0]);
    return -1;
  }

  return fds[0];
}

#ifdef SYS_CONSOLE
int
sys_launch_title(const char *title_id, const char *args) {
  app_launch_ctx_t ctx = { 0 };
  char *argv[255];
  int argc = 0;
  int app_id;
  int err;

  if(!args) {
    args = "";
  }

  LOG_PRINTF("launch title: %s %s\n", title_id, args);

  if((err = sceUserServiceGetForegroundUser(&ctx.user_id))) {
    LOG_PERROR("sceUserServiceGetForegroundUser");
    return err;
  }

  if((app_id = BIGAPP_ID()) > 0) {
    if((err = sceSystemServiceKillApp(app_id, -1, 0, 0))) {
      LOG_PERROR("sceSystemServiceKillApp");
      return err;
    }
  }

  argc = args_split(args, argv, 255);
  if((err = sceSystemServiceLaunchApp(title_id, argv, &ctx)) < 0) {
    LOG_PERROR("sceSystemServiceLaunchApp");
  }

  for(int i = 0; i < argc; i++) {
    free(argv[i]);
  }

  return err;
}

int
sys_notify_address(const char *prefix, int port) {
  char ip[INET_ADDRSTRLEN] = "127.0.0.1";
  char ifname[IFNAMSIZ] = "lo0";
  struct ifaddrs *ifaddr;
  int found = 0;

  if(getifaddrs(&ifaddr) == -1) {
    LOG_PERROR("getifaddrs");
    return -1;
  }

  for(struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    char addr[INET_ADDRSTRLEN];
    int preferred;

    if(ifa->ifa_addr == NULL) {
      continue;
    }

    if(ifa->ifa_addr->sa_family != AF_INET) {
      continue;
    }

    if(!strncmp("lo", ifa->ifa_name, 2)) {
      continue;
    }

    struct sockaddr_in *in = (struct sockaddr_in *)ifa->ifa_addr;
    inet_ntop(AF_INET, &(in->sin_addr), addr, sizeof(addr));

    if(!strncmp("0.", addr, 2)) {
      continue;
    }

    preferred
        = !strncmp("wlan", ifa->ifa_name, 4) || !strncmp("eth", ifa->ifa_name, 3);
    if(!found || preferred) {
      snprintf(ip, sizeof(ip), "%s", addr);
      snprintf(ifname, sizeof(ifname), "%s", ifa->ifa_name);
      found = 1;
      if(preferred) {
        break;
      }
    }
  }

  freeifaddrs(ifaddr);

  notify("%s %s:%d (%s)", prefix, ip, port, ifname);
  printf("%s %s:%d (%s)\n", prefix, ip, port, ifname);

  return 0;
}

void
on_fatal_signal(int sig) {
  void *buf[0x1000];
  int nptrs;

  notify("websrv.elf: %s", strsignal(sig));

  nptrs = backtrace(buf, sizeof(buf));
  backtrace_symbols_fd(buf, nptrs, open("/dev/console", O_WRONLY));

  _exit(EXIT_FAILURE);
}
#endif
