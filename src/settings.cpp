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
#include "settings.h"
#include "conf.h"
#include "log.h"

Settings Settings::settings;
int Settings::import_conf_file(const char *filename)
{
    const char *section, *key, *value;
    size_t section_len, key_len, value_len;
    ConfFile conf(filename);

    while (conf.next(&section, &section_len, &key, &key_len, &value, &value_len) == 0) {
        sections[std::string(section, section_len)][std::string(key, key_len)]
            = std::string(value, value_len);
    }

    return 0;
}

std::set<std::string> Settings::get_value_as_set(std::string section, std::string key)
{
    std::set<std::string> value_set;
    std::string &value = sections[section][key];
    size_t i = 0, j;

    j = value.find(' ');
    while (j != std::string::npos) {
        value_set.insert(value.substr(i, j - i));
        i = j + 1;
        j = value.find(' ', j + 1);
    }
    value_set.insert(value.substr(i, j - i));

    return value_set;
}
