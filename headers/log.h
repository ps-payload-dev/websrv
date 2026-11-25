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

#pragma once

#include <errno.h>
#include <stdio.h>
#include <string.h>

#if defined(__PROSPERO__)
#include <ps5/klog.h>
#elif defined(__ORBIS__)
#include <ps4/klog.h>
#else
#define klog_printf(s, ...) fprintf(stderr, s, ##__VA_ARGS__)
#endif

#define LOG_PUTS(s)                                                           \
  do {                                                                        \
    klog_printf("%s\n", s);                                                   \
  } while(0)

#define LOG_PRINTF(s, ...)                                                    \
  do {                                                                        \
    klog_printf(s, ##__VA_ARGS__);                                            \
  } while(0)

#define LOG_EPUTS(s)                                                          \
  do {                                                                        \
    klog_printf("%s:%d: %s\n", __FILE__, __LINE__, s);                        \
  } while(0)

#define LOG_EPRINTF(s, ...)                                                   \
  do {                                                                        \
    klog_printf("%s:%d: " s, __FILE__, __LINE__, ##__VA_ARGS__);              \
  } while(0)

#define LOG_PERROR(s)                                                         \
  do {                                                                        \
    klog_printf("%s:%d:%s: %s\n", __FILE__, __LINE__, s, strerror(errno));    \
  } while(0)
