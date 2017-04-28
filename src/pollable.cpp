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
#include <string.h>

#include "log.h"
#include "mainloop.h"
#include "pollable.h"

#define DEFAULT_BUF_SIZE 1024

Pollable::Pollable()
    : fd(0)
    , read_handler(0)
    , write_handler(0)
    , read_buf{DEFAULT_BUF_SIZE, new uint8_t[DEFAULT_BUF_SIZE]}
    , write_buf{0, nullptr}
    , read_cb([](const struct buffer &buf) {})
{
}

bool Pollable::can_read(void *data, int flags)
{
    assert(data);

    Pollable *p = (Pollable *)data;
    log_debug("can read");

    int r = p->do_read(p->read_buf);
    if (r == -EAGAIN) {
        log_debug("Read package failed. Trying again.");
        return true;
    }
    if (r < 0) {
        p->read_handler = 0;
        return false;
    }
    if (r > 0) {
        struct buffer tmp_buf = {(unsigned int)r, p->read_buf.data};
        p->read_cb(tmp_buf);
    }
    return true;
}

bool Pollable::can_write(void *data, int flags)
{
    assert(data);

    Pollable *p = (Pollable *)data;
    int r;

    if (!p->write_buf.data)
        goto end;

    r = p->do_write(p->write_buf);
    if (r == -EAGAIN) {
        log_debug("Write package failed. Trying again.");
        return true;
    }

    delete[] p->write_buf.data;
    p->write_buf = {0, nullptr};
end:
    p->write_handler = 0;
    return false;
}

void Pollable::monitor_read(bool monitor)
{
    Mainloop *m = Mainloop::get_mainloop();

    if (read_handler)
        m->remove_fd(read_handler);

    if (monitor)
        read_handler = m->add_fd(fd, Mainloop::IO_IN, can_read, this);
    else
        read_handler = 0;
}

int Pollable::write(const struct buffer &buf)
{
    int r;
    r = do_write(buf);
    if (r >= 0)
        return 0;

    if (write_buf.data) {
        delete[] write_buf.data;
        log_warning("There was another packet to be sent. Dropping that packet.");
    }

    write_buf = {buf.len, new uint8_t[buf.len]};
    memcpy(write_buf.data, buf.data, buf.len);

    if (!write_handler)
        write_handler = Mainloop::get_mainloop()->add_fd(fd, Mainloop::IO_OUT, can_write, this);
    return 0;
}

void Pollable::set_read_callback(std::function<void(const struct buffer &buf)> cb)
{
    read_cb = cb;
}
