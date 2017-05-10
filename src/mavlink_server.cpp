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
#include <mavlink.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"
#include "mainloop.h"
#include "mavlink_server.h"

#define DEFAULT_MAVLINK_PORT 14550
#define DEFAULT_MAVLINK_BROADCAST_ADDR "255.255.255.255"

MavlinkServer::MavlinkServer(std::vector<std::unique_ptr<Stream>> &streams)
    : _streams(streams)
    , _is_running(false)
    , _timeout_handler(0)
{
}

MavlinkServer::~MavlinkServer()
{
    stop();
}

void MavlinkServer::_handle_camera_info_request(unsigned int camera_id)
{
    uint8_t buffer[1024];
    struct buffer buf = {0, buffer};
    mavlink_message_t msg;

    log_debug("camera id %d", camera_id);
    for (auto const &s : _streams) {
        if (camera_id == 0 || camera_id == s->id) {
            mavlink_msg_camera_information_pack(
                _get_system_id(), MAV_COMP_ID_CAMERA, &msg, 0, s->id, 1,
                (const uint8_t *)"",
                (const uint8_t *)s->get_name().c_str(), 0, 0, 0, 0, 0, 0, 0);
        }

        buf.len = mavlink_msg_to_send_buffer(buf.data, &msg);
        if (!buf.len || _udp.write(buf) < 0) {
            log_error("Sending camera information failed for camera %d.", s->id);
            return;
        }
    }
}

void MavlinkServer::_handle_mavlink_message(mavlink_message_t *msg)
{
    log_debug("Message received: (sysid: %d compid: %d msgid: %d)", msg->sysid, msg->compid,
              msg->msgid);

    if (msg->msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
        mavlink_command_long_t cmd;
        mavlink_msg_command_long_decode(msg, &cmd);

        if (cmd.target_system != _get_system_id() || cmd.target_component != MAV_COMP_ID_CAMERA)
            return;

        switch (cmd.command) {
        case MAV_CMD_REQUEST_CAMERA_INFORMATION:
            this->_handle_camera_info_request(cmd.param2 /* Camera ID */);
            break;
        default:
            log_debug("Command %d unhandled. Discarding.", cmd.command);
        }
    }
}

void MavlinkServer::_message_received(const struct buffer &buf)
{
    mavlink_message_t msg;
    mavlink_status_t status;

    for (unsigned int i = 0; i < buf.len; ++i) {
        //TOOD: Parse mavlink message all at once, instead of using mavlink_parse_char
        if (mavlink_parse_char(MAVLINK_COMM_0, buf.data[i], &msg, &status))
            _handle_mavlink_message(&msg);
    }
}

bool _heartbeat_cb(void *data)
{
    assert(data);
    uint8_t buffer[1024];
    struct buffer buf = {0, buffer};

    MavlinkServer *server = (MavlinkServer *)data;
    mavlink_message_t msg;

    mavlink_msg_heartbeat_pack(1, MAV_COMP_ID_CAMERA, &msg, MAV_TYPE_GENERIC, MAV_AUTOPILOT_INVALID,
                               MAV_MODE_PREFLIGHT, 0, MAV_STATE_ACTIVE);
    // TODO: pack this in one function. Maybe in a class called MAVLinkSocket.
    buf.len = mavlink_msg_to_send_buffer(buf.data, &msg);
    if (!buf.len || server->_udp.write(buf) < 0)
        log_error("Sending HEARTBEAT failed.");
    else
        log_debug("HEARTBEAT");

    return true;
}

void MavlinkServer::start()
{
    if (_is_running)
        return;
    _is_running = true;

    _udp.open(DEFAULT_MAVLINK_BROADCAST_ADDR, DEFAULT_MAVLINK_PORT, false);
    _udp.set_read_callback([this](const struct buffer &buf) { this->_message_received(buf); });
    _timeout_handler = Mainloop::get_mainloop()->add_timeout(1000, _heartbeat_cb, this);
}

void MavlinkServer::stop()
{
    if (!_is_running)
        return;
    _is_running = false;

    if (_timeout_handler > 0)
        Mainloop::get_mainloop()->del_timeout(_timeout_handler);
}

int MavlinkServer::_get_system_id()
{
    return 1;
}
