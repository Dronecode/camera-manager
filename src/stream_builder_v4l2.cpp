/*
 * This file is part of the Camera Streaming Daemon
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

#include <assert.h>
#include <cstring>
#include <set>
#include <sys/types.h>

#include "conf_file.h"
#include "stream_builder_v4l2.h"
#include "stream_v4l2.h"

#define DEVICE_PATH "/dev/"
#define VIDEO_PREFIX "video"

static StreamBuilderV4l2 stream_builder;

static int parse_stl_set(const char *val, size_t val_len, void *storage, size_t storage_len)
{
    assert(val);
    assert(val_len);
    assert(storage);

    std::set<std::string> *value_set = (std::set<std::string> *)storage;
    char *end;

    while ((end = (char *)memchr((void *)val, ' ', val_len))) {
        if (end - val)
            value_set->insert(std::string(val, end - val));
        val_len = val_len - (end - val) - 1;
        val = end + 1;
    }
    if (val_len)
        value_set->insert(std::string(val, val_len));

    return 0;
}

std::vector<Stream *> StreamBuilderV4l2::build_streams(ConfFile &conf)
{
    DIR *dir;
    struct dirent *f;
    std::vector<Stream *> streams;
    std::set<std::string> blacklist;

    static const ConfFile::OptionsTable option_table[] = {
        {"blacklist", false, parse_stl_set, 0, 0},
    };

    conf.extract_options("v4l2", option_table, 1, (void *)&blacklist);

    if ((dir = opendir(DEVICE_PATH)) == NULL) {
        log_error("Unable to load v4l2 cameras");
        return streams;
    }

    while ((f = readdir(dir)) != NULL) {
        if (std::strncmp(VIDEO_PREFIX, f->d_name, sizeof(VIDEO_PREFIX) - 1) == 0) {
            std::string dev_path = DEVICE_PATH;
            // Don't add stream if it is in a blacklist
            if (blacklist.find(f->d_name) != blacklist.end())
                continue;
            dev_path.append(f->d_name);
            std::string path = "/";
            path.append(f->d_name);

            streams.push_back(new StreamV4l2(path, dev_path));
        }
    }
    closedir(dir);

    return streams;
}
