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
#include <map>
#include <set>

class Settings {
public:
    ~Settings(){};
    static Settings &get_instance() { return settings; };

    int import_conf_file(const char *filename);
    std::map<std::string, std::map<std::string, std::string>> &get_sections() { return sections; };
    std::set<std::string> get_value_as_set(std::string section, std::string key);

private:
    std::map<std::string, std::map<std::string, std::string>> sections;
    Settings() {}
    static Settings settings;
};
