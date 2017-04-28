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
#pragma once

#include <avahi-common/watch.h>
#include <functional>

struct buffer {
    unsigned int len;
    uint8_t *data;
};

class Pollable {
public:
    Pollable();
    virtual ~Pollable() {}

    int write(const struct buffer &buf);
    void monitor_read(bool monitor);
    void set_read_callback(std::function<void(const struct buffer &buf)> cb);

protected:
    int fd;
    virtual int do_write(const struct buffer &buf) = 0;
    virtual int do_read(const struct buffer &buf) = 0;

private:
    int read_handler, write_handler;
    void *buf_data;
    size_t buf_data_len;
    struct buffer read_buf, write_buf;

    static bool can_read(void *data, int flags);
    static bool can_write(void *data, int flags);
    std::function<void(const struct buffer &buf)> read_cb;
};
