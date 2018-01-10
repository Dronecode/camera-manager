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
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"
#include "socket.h"

#define DEFAULT_BUF_SIZE 1024

Socket::Socket()
    : _read_cb([](const struct buffer &buf, const struct sockaddr_in &sockaddr) {})
    , _write_buf{0, nullptr}
{
}

Socket::~Socket()
{
    free(_write_buf.data);
}

int Socket::write(const struct buffer &buf, const struct sockaddr_in &sockaddr)
{
    int r;
    r = _do_write(buf, sockaddr);
    if (r >= 0)
        return r;

    if (_write_buf.data) {
        delete[] _write_buf.data;
        log_warning("There was another packet to be sent. Dropping that packet.");
    }

    _write_buf = {buf.len, new uint8_t[buf.len]};
    memcpy(_write_buf.data, buf.data, buf.len);
    sockaddr_buf = sockaddr;
    monitor_write(true);

    return 0;
}

void Socket::set_read_callback(
    std::function<void(const struct buffer &buf, const struct sockaddr_in &sockaddr)> cb)
{
    _read_cb = cb;
}

bool Socket::_can_read()
{
    struct buffer read_buf {
        DEFAULT_BUF_SIZE, new uint8_t[DEFAULT_BUF_SIZE]
    };
    struct sockaddr_in sockaddr = {};
    int r = _do_read(read_buf, sockaddr);

    if (r == -EAGAIN) {
        log_debug("Read packet failed. Trying again.");
        goto end;
    }
    if (r < 0) {
        log_debug("Read failed. Droping packet.");
        delete[] read_buf.data;
        return false;
    }
    if (r > 0) {
        read_buf.len = (unsigned int)r;
        _read_cb(read_buf, sockaddr);
    }

end:
    delete[] read_buf.data;
    return true;
}

bool Socket::_can_write()
{
    int r;

    if (!_write_buf.data)
        return false;

    r = _do_write(_write_buf, sockaddr_buf);
    if (r == -EAGAIN) {
        log_debug("Write package failed. Trying again.");
        return true;
    } else if (r < 0) {
        log_error("Write package failed. Droping packet.");
    }

    delete[] _write_buf.data;
    _write_buf = {0, nullptr};
    return false;
}

UDPSocket::UDPSocket()
{
}

UDPSocket::~UDPSocket()
{
    close();
}

void UDPSocket::close()
{
    if (_fd >= 0)
        ::close(_fd);
    _fd = -1;
}

int UDPSocket::open(bool broadcast)
{
    const int broadcast_val = 1;

    _fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (_fd == -1) {
        log_error("Could not create socket (%m)");
        return -1;
    }

#ifdef ENABLE_GAZEBO
    // bind socket to port
    if (bind("0.0.0.0", 34550) == -1) {
        log_error("bind");
    }
#endif

    if (broadcast) {
        if (setsockopt(_fd, SOL_SOCKET, SO_BROADCAST, &broadcast_val, sizeof(broadcast_val))) {
            log_error("Error enabling broadcast in socket (%m)");
            goto fail;
        }
    }

    if (fcntl(_fd, F_SETFL, O_NONBLOCK | FASYNC) < 0) {
        log_error("Error setting socket _fd as non-blocking (%m)");
        goto fail;
    }

    log_info("Open UDP [%d]", _fd);

    monitor_read(true);
    return _fd;

fail:
    if (_fd >= 0) {
        ::close(_fd);
        _fd = -1;
    }
    return -1;
}

int UDPSocket::bind(const char *ip, unsigned long port)
{
    assert(ip);

    struct sockaddr_in sockaddr = {};

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(ip);
    sockaddr.sin_port = htons(port);

    if (_fd < 0) {
        log_error("Trying to bind an invalid _fd");
        return -EINVAL;
    }

    if (::bind(_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
        log_error("Error binding socket (%m)");
        return -1;
    }

    log_info("Bind UDP [%d] %s:%lu", _fd, ip, port);
    return 0;
}

int UDPSocket::_do_write(const struct buffer &buf, const struct sockaddr_in &sockaddr)
{
    if (_fd < 0) {
        log_error("Trying to write to an invalid _fd");
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

int UDPSocket::_do_read(const struct buffer &buf, struct sockaddr_in &sockaddr)
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
