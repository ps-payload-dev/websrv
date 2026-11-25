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

#define SYS_exit 1
#define SYS_read 3
#define SYS_write 4
#define SYS_open 5
#define SYS_close 6
#define SYS_socket 97
#define SYS_connect 98
#define SYS_nanosleep 240

#define WEBSRV_PORT 8080

#define AF_INET 2
#define SOCK_STREAM 1
#define O_RDONLY 0

#define LOADER_PORT 9021

long
__syscall(long n, ...) {
  long a1 = 0, a2 = 0, a3 = 0, a4 = 0, a5 = 0, a6 = 0;
  __builtin_va_list ap;
  unsigned long ret;
  char iserror;

  __builtin_va_start(ap, n);
  a1 = __builtin_va_arg(ap, long);
  a2 = __builtin_va_arg(ap, long);
  a3 = __builtin_va_arg(ap, long);
  a4 = __builtin_va_arg(ap, long);
  a5 = __builtin_va_arg(ap, long);
  a6 = __builtin_va_arg(ap, long);
  __builtin_va_end(ap);

  register long r10 __asm__("r10") = a4;
  register long r8 __asm__("r8") = a5;
  register long r9 __asm__("r9") = a6;

  __asm__ __volatile__("syscall"
                       : "=a"(ret), "=@ccc"(iserror), "+r"(r10), "+r"(r8),
                         "+r"(r9)
                       : "a"(n), "D"(a1), "S"(a2), "d"(a3)
                       : "rcx", "r11", "memory");

  return iserror ? -ret : ret;
}

static int
klog(const char *s) {
  return __syscall(0x259, 7, s, 0);
}

static int
send_all(int fd, const char *buf, long len) {
  while(len > 0) {
    long n = __syscall(SYS_write, fd, buf, len);
    if(n <= 0) {
      return -1;
    }
    buf += n;
    len -= n;
  }

  return 0;
}

static void
sleep_ms(long ms) {
  long ts[2];

  ts[0] = ms / 1000;
  ts[1] = (ms % 1000) * 1000000L;
  __syscall(SYS_nanosleep, ts, 0);
}

static int
connect_port(int port) {
  struct {
    unsigned char sin_len;
    unsigned char sin_family;
    unsigned short sin_port;
    unsigned int sin_addr;
    unsigned char sin_zero[8];
  } addr;
  unsigned char *p = (unsigned char *)&addr;
  int fd;

  for(unsigned i = 0; i < sizeof(addr); i++) {
    p[i] = 0;
  }

  addr.sin_len = sizeof(addr);
  addr.sin_family = AF_INET;
  addr.sin_port = (unsigned short)(((port & 0xff) << 8) | (port >> 8));
  addr.sin_addr = 0x0100007f;

  if((fd = __syscall(SYS_socket, AF_INET, SOCK_STREAM, 0)) < 0) {
    return -1;
  }

  if(__syscall(SYS_connect, fd, &addr, sizeof(addr)) < 0) {
    __syscall(SYS_close, fd);
    return -1;
  }

  return fd;
}

static int
send_to_loader(const char *app0, const char *path) {
  char buf[4096];
  int in, out;
  long n;
  int err = 0;

  if((in = __syscall(SYS_open, app0, O_RDONLY, 0)) < 0
     && (in = __syscall(SYS_open, path, O_RDONLY, 0)) < 0) {
    klog("websrv: unable to open the payload\n");
    return -1;
  }

  if((out = connect_port(LOADER_PORT)) < 0) {
    klog("websrv: unable to connect to the ELF loader\n");
    __syscall(SYS_close, in);
    return -1;
  }

  while((n = __syscall(SYS_read, in, buf, sizeof(buf))) > 0) {
    if(send_all(out, buf, n)) {
      klog("websrv: failed to send the payload\n");
      err = -1;
      break;
    }
  }

  __syscall(SYS_close, in);
  __syscall(SYS_close, out);

  return err;
}

static int
wait_for_websrv(void) {
  int fd;

  for(int i = 0; i < 60; i++) {
    if((fd = connect_port(WEBSRV_PORT)) >= 0) {
      __syscall(SYS_close, fd);
      return 0;
    }
    sleep_ms(500);
  }

  return -1;
}

int
main(void) {
  int fd;

  klog("websrv: eboot started\n");

  if((fd = connect_port(WEBSRV_PORT)) >= 0) {
    __syscall(SYS_close, fd);
    klog("websrv: already serving\n");

  } else {
    klog("websrv: not serving, starting it\n");
    if(send_to_loader("/app0/websrv.elf",
                      "/user/app/" TITLE_ID "/websrv.elf")) {
      return 1;
    }
    klog("websrv: sent websrv.elf\n");

    if(wait_for_websrv()) {
      klog("websrv: not serving after being started\n");
      return 1;
    }
    klog("websrv: serving\n");
  }

  if(send_to_loader("/app0/open.elf", "/user/app/" TITLE_ID "/open.elf")) {
    return 1;
  }
  klog("websrv: sent open.elf\n");

  return 0;
}

void
_start(void) {
  __syscall(SYS_exit, main());
  __builtin_trap();
}
