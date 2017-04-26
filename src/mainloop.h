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

class Mainloop {
public:
    virtual void loop() = 0;
    virtual const AvahiPoll *get_avahi_poll_api() = 0;
    static Mainloop *get_mainloop() { return mainloop; };
    virtual unsigned int add_timeout(unsigned int timeout_msec, bool (*cb)(void *),
                                     const void *data) = 0;
    virtual void del_timeout(unsigned int timeout_handler) = 0;
    virtual int add_fd(int fd, int flags, bool (*cb)(void *data, int flags), void *data) = 0;
    virtual void remove_fd(int handler) = 0;
    virtual void quit() = 0;

    enum io_flags { IO_IN = 1, IO_OUT = 2 };

protected:
    static Mainloop *mainloop;
};
