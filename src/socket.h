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

struct buffer {
    unsigned int len;
    uint8_t *data;
};

class Socket : public Pollable {
public:
    Socket();
    virtual ~Socket();

    int write(const struct buffer &buf, const struct sockaddr_in &sockaddr);
    void set_read_callback(
        std::function<void(const struct buffer &buf, const struct sockaddr_in &sockaddr)> cb);

protected:
    bool _can_read() override;
    bool _can_write() override;
    virtual int _do_write(const struct buffer &buf, const struct sockaddr_in &sockaddr) = 0;
    virtual int _do_read(const struct buffer &buf, struct sockaddr_in &sockaddr) = 0;

private:
    std::function<void(const struct buffer &buf, const struct sockaddr_in &sockaddr)> _read_cb;
    struct buffer _write_buf;
    struct sockaddr_in sockaddr_buf {};
};

class UDPSocket : public Socket {
public:
    UDPSocket();
    ~UDPSocket();

    int open(bool broadcast);
    void close();
    int bind(const char *addr, unsigned long port);

protected:
    int _do_write(const struct buffer &buf, const struct sockaddr_in &sockaddr) override;
    int _do_read(const struct buffer &buf, struct sockaddr_in &sockaddr) override;
};
