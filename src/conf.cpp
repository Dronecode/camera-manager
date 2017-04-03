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

#include "conf.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

struct config {
    char *key;
    char *value;
    struct config *next;
};

struct section {
    char *name;
    struct config *configs;
    struct config *current_config;
    struct section *next;
};

ConfFile::ConfFile(const char *filename)
    : _filename(filename)
    , _entry(nullptr)
    , _last_section(nullptr)
    , _n(0)
    , _line(0)
{
    assert(filename);

    _file = fopen(filename, "re");
}

ConfFile::~ConfFile()
{
    free(_entry);
    free(_last_section);
    if (_file)
        fclose(_file);
}

int ConfFile::next(const char **section, size_t *section_len, const char **key, size_t *key_len,
                   const char **value, size_t *value_len)
{
    assert(section);
    assert(section_len);
    assert(key);
    assert(key_len);

    int ret = EOF;
    ssize_t read;
    const char *str;
    size_t len = 0;

    if (!_file) {
        log_error("Could not open conf file '%s'", _filename);
        return -EIO;
    }

    while ((read = getline(&_entry, &_n, _file)) >= 0) {
        _line++;
        len = strlen(_entry);
        if (!len)
            continue;

        str = _entry;
        _trim(&str, &len);
        switch (str[0]) {
        case ';':
        case '#':
        case '\0':
            // Discards comment or blank line
            break;
        case '[':
            free(_last_section);
            _last_section = _entry;
            _entry = NULL;
            ret = _trim_section(&str, &len);
            if (ret < 0)
                goto end;
            *section = str;
            *section_len = len;
            break;
        default:
            *key = str;
            *key_len = len;
            ret = _parse_key_value(key, key_len, value, value_len);
            if (ret < 0)
                goto end;
            return 0;
        }
    }

end:
    return ret;
}

int ConfFile::_trim_section(const char **entry, size_t *len)
{
    if ((*entry)[*len - 1] != ']') {
        log_error("On file %s: Line %d: Unfinished section name. Expected ']'", _filename, _line);
        return -EINVAL;
    }

    // So section name is the string between '[]'
    (*entry)++;
    *len -= 2;
    _trim(entry, len);

    return 0;
}

int ConfFile::_parse_key_value(const char **key, size_t *key_len, const char **value,
                               size_t *value_len)
{
    const char *equal_pos;

    if (!(equal_pos = (const char *)memchr(*key, '=', *key_len))) {
        log_error("On file %s: Line %d: Missing '=' on config", _filename, _line);
        return -EINVAL;
    }

    if (equal_pos == *key) {
        log_error("On file %s: Line %d: Missing name on config", _filename, _line);
        return -EINVAL;
    }

    if (equal_pos == (*key + *key_len - 1)) {
        log_error("On file %s: Line %d: Missing value on config", _filename, _line);
        return -EINVAL;
    }

    *value = equal_pos + 1;
    *value_len = *key_len - (equal_pos - *key) - 1;
    *key_len = equal_pos - *key;

    _trim(value, value_len);
    _trim(key, key_len);

    return 0;
}

void ConfFile::_trim(const char **str, size_t *len)
{
    const char *s = *str;
    const char *end = s + *len;

    while (isspace(*s))
        s++;

    while (end != s && isspace(*(end - 1)))
        end--;

    *len = end - s;
    *str = s;
}
