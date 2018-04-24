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
#pragma once

#ifdef ENABLE_AVAHI
#include <avahi-common/watch.h>
#endif
#include <functional>

class Pollable {
public:
    Pollable();
    virtual ~Pollable() {}

    void monitor_read(bool monitor);
    void monitor_write(bool monitor);

protected:
    int _fd;
    virtual bool _can_read() = 0;
    virtual bool _can_write() = 0;

private:
    int _read_handler, _write_handler;
    static bool _can_read_cb(const void *data, int flags);
    static bool _can_write_cb(const void *data, int flags);
};
