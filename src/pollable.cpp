/*
 * This file is part of the Dronecode Camera Manager
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

#include "log.h"
#include "mainloop.h"
#include "pollable.h"

Pollable::Pollable()
    : _fd(-1)
    , _read_handler(0)
    , _write_handler(0)
{
}

bool Pollable::_can_read_cb(const void *data, int flags)
{
    assert(data);

    Pollable *p = (Pollable *)data;

    if (p->_can_read())
        return true;

    p->monitor_read(false);
    return false;
}

bool Pollable::_can_write_cb(const void *data, int flags)
{
    assert(data);

    Pollable *p = (Pollable *)data;

    if (p->_can_write())
        return true;

    p->monitor_write(false);
    return false;
}

void Pollable::monitor_read(bool monitor)
{
    Mainloop *m = Mainloop::get_mainloop();

    if (_read_handler)
        m->remove_fd(_read_handler);

    if (monitor)
        _read_handler = m->add_fd(_fd, Mainloop::IO_IN, _can_read_cb, this);
    else
        _read_handler = 0;
}

void Pollable::monitor_write(bool monitor)
{
    Mainloop *m = Mainloop::get_mainloop();

    if (_write_handler)
        m->remove_fd(_write_handler);

    if (monitor)
        _write_handler = m->add_fd(_fd, Mainloop::IO_OUT, _can_write_cb, this);
    else
        _write_handler = 0;
}
