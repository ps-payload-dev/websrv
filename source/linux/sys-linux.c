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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "sys.h"

int
sys_launch_title(const char *title_id, const char *args) {
  printf("launch title: %s %s\n", title_id, args ? args : "");
  return -1;
}

pid_t
sys_spawn(const char *cwd, int stdio, char *const argv[], char *const envp[],
          const uint8_t *elf, size_t elf_size) {
  char filename[] = "/tmp/elfXXXXXX";
  pid_t pid;
  FILE *f;
  int fd;

  if((fd = mkstemp(filename)) < 0) {
    perror("mkstemp");
    return -1;
  }

  if(!(f = fdopen(fd, "wb"))) {
    perror("fdopen");
    close(fd);
    unlink(filename);
    return -1;
  }

  if(fwrite(elf, elf_size, 1, f) != 1) {
    perror("fwrite");
    fclose(f);
    unlink(filename);
    return -1;
  }
  fclose(f);
  chmod(filename, 0700);

  if((pid = fork()) < 0) {
    perror("fork");
    unlink(filename);
    return -1;
  }

  if(!pid) {
    if(cwd && chdir(cwd)) {
      _exit(127);
    }
    dup2(stdio, STDOUT_FILENO);
    dup2(stdio, STDERR_FILENO);
    if(envp && envp[0]) {
      execve(filename, argv, envp);
    } else {
      execv(filename, argv);
    }
    _exit(127);
  }

  return pid;
}
