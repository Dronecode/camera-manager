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
    : _fd(0)
    , _read_handler(0)
    , _write_handler(0)
    , _read_buf{DEFAULT_BUF_SIZE, new uint8_t[DEFAULT_BUF_SIZE]}
    , _write_buf{0, nullptr}
    , _read_cb([](const struct buffer &buf) {})
{
}

bool Pollable::_can_read(const void *data, int flags)
{
    assert(data);

    Pollable *p = (Pollable *)data;

    int r = p->_do_read(p->_read_buf);
    if (r == -EAGAIN) {
        log_debug("Read package failed. Trying again.");
        return true;
    }
    if (r < 0) {
        p->_read_handler = 0;
        return false;
    }
    if (r > 0) {
        struct buffer tmp_buf = {(unsigned int)r, p->_read_buf.data};
        p->_read_cb(tmp_buf);
    }
    return true;
}

bool Pollable::_can_write(const void *data, int flags)
{
    assert(data);

    Pollable *p = (Pollable *)data;
    int r;

    if (!p->_write_buf.data)
        goto end;

    r = p->_do_write(p->_write_buf);
    if (r == -EAGAIN) {
        log_debug("Write package failed. Trying again.");
        return true;
    }

    delete[] p->_write_buf.data;
    p->_write_buf = {0, nullptr};
end:
    p->_write_handler = 0;
    return false;
}

void Pollable::monitor_read(bool monitor)
{
    Mainloop *m = Mainloop::get_mainloop();

    if (_read_handler)
        m->remove_fd(_read_handler);

    if (monitor)
        _read_handler = m->add_fd(_fd, Mainloop::IO_IN, _can_read, this);
    else
        _read_handler = 0;
}

int Pollable::write(const struct buffer &buf)
{
    int r;
    r = _do_write(buf);
    if (r >= 0)
        return 0;

    if (_write_buf.data) {
        delete[] _write_buf.data;
        log_warning("There was another packet to be sent. Dropping that packet.");
    }

    _write_buf = {buf.len, new uint8_t[buf.len]};
    memcpy(_write_buf.data, buf.data, buf.len);

    if (!_write_handler)
        _write_handler = Mainloop::get_mainloop()->add_fd(_fd, Mainloop::IO_OUT, _can_write, this);
    return 0;
}

void Pollable::set_read_callback(std::function<void(const struct buffer &buf)> cb)
{
    _read_cb = cb;
}
