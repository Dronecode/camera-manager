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

#include <map>
#include <mavlink.h>
#include <memory>
#include <vector>

#include "conf_file.h"
#include "rtsp_server.h"
#include "socket.h"
#include "stream.h"

class MavlinkServer {
public:
    MavlinkServer(ConfFile &conf, std::vector<std::unique_ptr<Stream>> &streams, RTSPServer &rtsp);
    ~MavlinkServer();
    void start();
    void stop();

private:
    const std::vector<std::unique_ptr<Stream>> &_streams;
    bool _is_running;
    unsigned int _timeout_handler;
    UDPSocket _udp;
    struct sockaddr_in _broadcast_addr = {};
    int _system_id;
    int _comp_id;
    char *_rtsp_server_addr;
    RTSPServer &_rtsp;

    void _message_received(const struct sockaddr_in &sockaddr, const struct buffer &buf);
    void _handle_mavlink_message(const struct sockaddr_in &addr, mavlink_message_t *msg);
    void _handle_camera_info_request(const struct sockaddr_in &addr, int command,
                                     unsigned int camera_id, unsigned int action);
    void _handle_camera_video_stream_request(const struct sockaddr_in &addr, int command,
                                             unsigned int camera_id, unsigned int action);
    void _handle_camera_set_video_stream_settings(const struct sockaddr_in &addr,
                                                  mavlink_message_t *msg);
    bool _send_mavlink_message(const struct sockaddr_in *addr, mavlink_message_t &msg);
    void _send_ack(const struct sockaddr_in &addr, int cmd, bool success);
    const Stream::FrameSize *_find_best_frame_size(Stream &s, uint32_t w, uint32_t v);
    friend bool _heartbeat_cb(void *data);
};
