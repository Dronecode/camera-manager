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

MavlinkServer::MavlinkServer(std::vector<std::unique_ptr<Stream>> &_streams)
    : streams(_streams)
    , is_running(false)
{
}

MavlinkServer::~MavlinkServer()
{
    stop();
}

void MavlinkServer::handle_camera_info_request(unsigned int camera_id)
{
    uint8_t buffer[1024];
    struct buffer buf = {0, buffer};
    mavlink_message_t msg;

    log_debug("camera id %d", camera_id);
    for (auto const &s : streams) {
        if (camera_id == 0 || camera_id == s->id) {
            mavlink_msg_camera_information_pack(
                getSystemID(), MAV_COMP_ID_CAMERA, &msg, 0, s->id, 1,
                (const uint8_t *)s->get_name().c_str() /* TODO: Get vendor name. */,
                (const uint8_t *)s->get_name().c_str(), 0, 0, 0, 0, 0, 0, 0);
        }

        buf.len = mavlink_msg_to_send_buffer(buf.data, &msg);
        if (!buf.len || udp.write(buf) < 0) {
            log_error("Sending camera information failed for camera %d.", s->id);
            return;
        }
    }
}

void MavlinkServer::handle_mavlink_message(mavlink_message_t *msg)
{
    log_debug("Message received: (sysid: %d compid: %d msgid: %d)", msg->sysid, msg->compid,
              msg->msgid);

    if (msg->msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
        mavlink_command_long_t cmd;
        mavlink_msg_command_long_decode(msg, &cmd);

        if (cmd.target_system != getSystemID() || cmd.target_component != MAV_COMP_ID_CAMERA)
            return;

        switch (cmd.command) {
        case MAV_CMD_REQUEST_CAMERA_INFORMATION:
            this->handle_camera_info_request(cmd.param2 /* Camera ID */);
            break;
        default:
            log_debug("Command %d unhandled. Discarding.", cmd.command);
        }
    }
}

void MavlinkServer::message_received(const struct buffer &buf)
{
    mavlink_message_t msg;
    mavlink_status_t status;

    for (unsigned int i = 0; i < buf.len; ++i) {
        if (mavlink_parse_char(MAVLINK_COMM_0, buf.data[i], &msg, &status))
            handle_mavlink_message(&msg);
    }
}

bool heartbeat_cb(void *data)
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
    if (!buf.len || server->udp.write(buf) < 0)
        log_error("Sending HEARTBEAT failed.");
    else
        log_debug("HEARTBEAT");

    return true;
}

void MavlinkServer::start()
{
    if (is_running)
        return;
    is_running = true;

    udp.open(DEFAULT_MAVLINK_BROADCAST_ADDR, DEFAULT_MAVLINK_PORT, false);
    udp.set_read_callback([this](const struct buffer &buf) { this->message_received(buf); });
    timeout_handler = Mainloop::get_mainloop()->add_timeout(1000, heartbeat_cb, this);
}

void MavlinkServer::stop()
{
    if (!is_running)
        return;
    is_running = false;

    Mainloop::get_mainloop()->del_timeout(timeout_handler);
}

int MavlinkServer::getSystemID()
{
    return 1;
}
