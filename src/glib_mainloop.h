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

#ifdef ENABLE_AVAHI
#include <avahi-glib/glib-watch.h>
#endif
#include <vector>

#include "mainloop.h"

class GlibMainloop : public Mainloop {
public:
    GlibMainloop();
    ~GlibMainloop();
    void loop() override;
    void quit() override;
#ifdef ENABLE_AVAHI
    const AvahiPoll *get_avahi_poll_api() override;
#endif

    unsigned int add_timeout(unsigned int timeout_msec, bool (*cb)(void *),
                             const void *data) override;
    void del_timeout(unsigned int timeout_handler) override;
    int add_fd(int fd, int flags, bool (*cb)(const void *data, int flags), const void *data) override;
    void remove_fd(int handler) override;

#ifdef ENABLE_AVAHI
private:
    AvahiGLibPoll *avahi_poll;
#endif
};
