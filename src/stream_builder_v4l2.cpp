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
#include <cstring>

#include "stream_builder_v4l2.h"
#include "stream_v4l2.h"

#define DEVICE_PATH "/dev/"
#define VIDEO_PREFIX "video"

static StreamBuilderV4l2 stream_builder;

std::vector<Stream *> StreamBuilderV4l2::build_streams()
{
    DIR *dir;
    struct dirent *f;
    std::vector<Stream *> streams;

    errno = 0;
    if ((dir = opendir(DEVICE_PATH)) == NULL) {
        log_error("Unable to load v4l2 cameras");
        return streams;
    }

    while ((f = readdir(dir)) != NULL) {
        if (std::strncmp(VIDEO_PREFIX, f->d_name, sizeof(VIDEO_PREFIX) - 1) == 0) {
            std::string dev_path = DEVICE_PATH;
            dev_path.append(f->d_name);
            std::string path = "/";
            path.append(f->d_name);

            streams.push_back(new StreamV4l2(gst_builder, path, dev_path));
        }
    }
    closedir(dir);

    return streams;
}
