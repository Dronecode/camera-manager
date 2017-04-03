/*
 * This file is part of the MAVLink Router project
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
#include <ctype.h>
#include <stdio.h>
#include <string>

class ConfFile {
public:
    ConfFile(const char *filename);

    ~ConfFile();

    int next(const char **section, size_t *section_len, const char **key, size_t *key_len,
             const char **value, size_t *value_len);

    int next(std::string &section_str, std::string &key_str, std::string &value_str);

private:
    void _trim(const char **str, size_t *len);
    int _trim_section(const char **entry, size_t *len);
    int _parse_key_value(const char **key, size_t *key_len, const char **value, size_t *value_len);
    FILE *_file;
    const char *_filename;
    char *_entry, *_last_section;
    size_t _n;
    int _line;
};
