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

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"
#include "socket.h"

UDPSocket::UDPSocket()
{
}

UDPSocket::~UDPSocket()
{
    ::close(_fd);
}

int UDPSocket::open(const char *ip, unsigned long port, bool to_bind)
{
    assert(ip);

    const int broadcast_val = 1;

    _fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (_fd == -1) {
        log_error("Could not create socket (%m)");
        return -1;
    }

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(ip);
    sockaddr.sin_port = htons(port);

    if (to_bind) {
        if (bind(_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
            log_error("Error binding socket (%m)");
            goto fail;
        }
    } else {
        if (setsockopt(_fd, SOL_SOCKET, SO_BROADCAST, &broadcast_val, sizeof(broadcast_val))) {
            log_error("Error enabling broadcast in socket (%m)");
            goto fail;
        }
    }

    if (fcntl(_fd, F_SETFL, O_NONBLOCK | FASYNC) < 0) {
        log_error("Error setting socket _fd as non-blocking (%m)");
        goto fail;
    }

    if (to_bind)
        sockaddr.sin_port = 0;
    log_info("Open UDP [%d] %s:%lu %c", _fd, ip, port, to_bind ? '*' : ' ');

    monitor_read(true);
    return _fd;

fail:
    if (_fd >= 0) {
        ::close(_fd);
        _fd = -1;
    }
    return -1;
}

int UDPSocket::_do_write(const struct buffer &buf)
{
    if (_fd < 0) {
        log_error("Trying to write invalid _fd");
        return -EINVAL;
    }

    if (!sockaddr.sin_port) {
        log_debug("No one ever connected to %d. No one to write for", _fd);
        return 0;
    }

    ssize_t r = ::sendto(_fd, buf.data, buf.len, 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (r == -1) {
        if (errno != EAGAIN && errno != ECONNREFUSED && errno != ENETUNREACH)
            log_error("Error sending udp packet (%m)");
        return -errno;
    };

    /* Incomplete packet, we warn and discard the rest */
    if (r != (ssize_t)buf.len) {
        log_debug("Discarding packet, incomplete write %zd but len=%u", r, buf.len);
    }

    log_debug("UDP: [%d] wrote %zd bytes", _fd, r);

    return r;
}

int UDPSocket::_do_read(const struct buffer &buf)
{
    socklen_t addrlen = sizeof(sockaddr);
    ssize_t r = ::recvfrom(_fd, buf.data, buf.len, 0, (struct sockaddr *)&sockaddr, &addrlen);
    if (r == -1) {
        if (errno != EAGAIN)
            log_error("Error reading udp packet (%m)");
        return -errno;
    }

    return r;
}
