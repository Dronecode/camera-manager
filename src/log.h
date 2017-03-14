/*
 * This file is part of the Camera Streaming Daemon project
 *
 * Copyright (C) 2017  Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <syslog.h>

#include "macro.h"

#ifdef __cplusplus
extern "C" {
#endif

int log_open(void);
int log_close(void);

int log_get_max_level(void) _pure_;
void log_set_max_level(int level);
int log_internal(int level, int error, const char *file, int line, const char *format, ...)
    _printf_format_(5, 6);

#define log_debug(...) log_internal(LOG_DEBUG, 0, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...) log_internal(LOG_INFO, 0, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_internal(LOG_ERR, 0, __FILE__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif
