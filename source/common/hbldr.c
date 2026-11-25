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

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"
#include "hbldr.h"
#include "log.h"
#include "sys.h"

#ifndef __PROSPERO__
pid_t
hbldr_spawn(const char *cwd, const char *path, int stdio, char **argv,
            char **envp, uint8_t *elf, size_t elf_size) {
  return sys_spawn(cwd, stdio, argv, envp, elf, elf_size);
}
#endif

pid_t
hbldr_launch(const char *cwd, const char *path, int stdio, char **argv,
             char **envp) {
  char buf[PATH_MAX];
  uint8_t *elf;
  size_t elf_size;
  pid_t pid;

  if(path[0] == '/') {
    snprintf(buf, sizeof(buf), "%s", path);
  } else {
    snprintf(buf, sizeof(buf), "%s/%s", cwd, path);
  }

  LOG_PRINTF("hbldr_launch: reading %s\n", buf);

  if(!(elf = fs_readfile(buf, &elf_size))) {
    LOG_PRINTF("hbldr_launch: fs_readfile failed for %s (errno=%d %s)\n", buf,
               errno, strerror(errno));
    return -1;
  }

  pid = hbldr_spawn(cwd, path, stdio, argv, envp, elf, elf_size);
  LOG_PRINTF("hbldr_launch: spawn returned pid=%d errno=%d\n", pid, errno);

  free(elf);

  return pid;
}
