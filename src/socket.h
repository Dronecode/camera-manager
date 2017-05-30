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

#include <arpa/inet.h>

#include "pollable.h"

class Socket : public Pollable {
public:
    Socket(){};
    virtual ~Socket(){};
};

class UDPSocket : public Socket {
public:
    UDPSocket();
    ~UDPSocket();

    int open(const char *addr, unsigned long port, bool to_bind);

protected:
    int _do_write(const struct buffer &buf) override;
    int _do_read(const struct buffer &buf) override;

private:
    struct sockaddr_in sockaddr {};
};
