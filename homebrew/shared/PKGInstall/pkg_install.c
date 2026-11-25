/* Copyright (C) 2025 John Törnblom
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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

int sceAppInstUtilInitialize(void);
int sceAppInstUtilAppInstallPkg(const char *, int);

#ifdef __ORBIS__
#define EXDEV_RESULT ((int)0x80020012)

static const char *
same_device_path(const char *path, char *buf, size_t size) {
  if(!strncmp(path, "/data/", 6)) {
    snprintf(buf, size, "/user/data/%s", path + 6);
    return buf;
  }

  return path;
}

/*
 * Copy the package to the device of /user/app, e.g., from a USB drive, and
 * return the path of the copy.
 */
static const char *
copy_to_same_device(const char *path, char *buf, size_t size) {
  const char *tmp = "/user/data/pkg_install_tmp.pkg";
  struct stat st;
  struct stat tst;
  char *chunk;
  ssize_t n;
  int in;
  int out;

  snprintf(buf, size, "%s", tmp);

  if(stat(path, &st)) {
    perror(path);
    return 0;
  }

  if(!stat(buf, &tst) && tst.st_dev == st.st_dev && tst.st_ino == st.st_ino) {
    return 0;
  }
  printf("Copying %s to %s (%lld bytes)\n", path, buf, (long long)st.st_size);

  if((in = open(path, O_RDONLY)) < 0) {
    perror(path);
    return 0;
  }
  if((out = open(buf, O_WRONLY | O_CREAT | O_TRUNC, 0777)) < 0) {
    perror(buf);
    close(in);
    return 0;
  }
  if(!(chunk = malloc(0x100000))) {
    close(in);
    close(out);
    return 0;
  }

  while((n = read(in, chunk, 0x100000)) > 0) {
    if(write(out, chunk, n) != n) {
      perror(buf);
      n = -1;
      break;
    }
  }

  free(chunk);
  close(in);
  close(out);

  if(n < 0) {
    unlink(buf);
    return 0;
  }

  return buf;
}
#endif

int
main(int argc, char **argv) {
  const char *path;
  int result;
#ifdef __ORBIS__
  char buf[4096];
#endif

  if(argc < 2) {
    printf("Usage: %s PKG_PATH\n", argv[0]);
    return -1;
  }

  if(sceAppInstUtilInitialize()) {
    return -1;
  }

  path = argv[1];
#ifdef __ORBIS__
  path = same_device_path(path, buf, sizeof(buf));
#endif

  result = sceAppInstUtilAppInstallPkg(path, 0);

#ifdef __ORBIS__
  if(result == EXDEV_RESULT
     && (path = copy_to_same_device(argv[1], buf, sizeof(buf)))) {
    result = sceAppInstUtilAppInstallPkg(path, 0);
    unlink(path);
  }
#endif

  printf("sceAppInstUtilAppInstallPkg(%s): 0x%x\n", argv[1], result);
  return result;
}
