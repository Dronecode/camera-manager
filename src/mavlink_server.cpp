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
#include "util.h"

#define DEFAULT_MAVLINK_PORT 14550
#define DEFAULT_SYSID 42
#define DEFAULT_MAVLINK_BROADCAST_ADDR "255.255.255.255"
#define MAX_MAVLINK_MESSAGE_SIZE 1024

MavlinkServer::MavlinkServer(ConfFile &conf, std::vector<std::unique_ptr<Stream>> &streams)
    : _streams(streams)
    , _is_running(false)
    , _timeout_handler(0)
    , _broadcast_addr{}
    , _system_id(DEFAULT_SYSID)
{
    struct options {
        unsigned long int port;
        int sysid;
        char broadcast[17];
    } opt = {};
    static const ConfFile::OptionsTable option_table[] = {
        {"port", false, ConfFile::parse_ul, OPTIONS_TABLE_STRUCT_FIELD(options, port)},
        {"system_id", false, ConfFile::parse_ul, OPTIONS_TABLE_STRUCT_FIELD(options, sysid)},
        {"broadcast_addr", false, ConfFile::parse_str_buf, OPTIONS_TABLE_STRUCT_FIELD(options, broadcast)},
    };
    conf.extract_options("mavlink", option_table, ARRAY_SIZE(option_table), (void *)&opt);

    if (opt.port)
        _broadcast_addr.sin_port = htons(opt.port);
    else
        _broadcast_addr.sin_port = htons(DEFAULT_MAVLINK_PORT);

    if (opt.sysid) {
        if (opt.sysid <= 1 || opt.sysid >= 255)
            log_error("Invalid System ID for MAVLink communication (%d). Using default (%d)",
                      opt.sysid, DEFAULT_SYSID);
        else
            _system_id = DEFAULT_SYSID;
    }

    if (opt.broadcast[0])
        _broadcast_addr.sin_addr.s_addr = inet_addr(opt.broadcast);
    else
        _broadcast_addr.sin_addr.s_addr = inet_addr(DEFAULT_MAVLINK_BROADCAST_ADDR);
    _broadcast_addr.sin_family = AF_INET;
}

MavlinkServer::~MavlinkServer()
{
    stop();
}

void MavlinkServer::_send_ack(const struct sockaddr_in &addr, int cmd, bool success)
{
    mavlink_message_t msg;

    mavlink_msg_command_ack_pack(_system_id, MAV_COMP_ID_CAMERA, &msg, cmd,
                                 success ? MAV_RESULT_ACCEPTED : MAV_RESULT_FAILED, 255);

    if (!_send_mavlink_message(&addr, msg)) {
        log_error("Sending ack failed.");
        return;
    }
}

void MavlinkServer::_handle_camera_info_request(const struct sockaddr_in &addr, int command,
                                                unsigned int camera_id, unsigned int action)
{
    mavlink_message_t msg;
    bool success = false;

    if (action != 1) {
        _send_ack(addr, command, true);
        return;
    }

    for (auto const &s : _streams) {
        if (camera_id == 0 || camera_id == s->id) {
            mavlink_msg_camera_information_pack(
                _system_id, MAV_COMP_ID_CAMERA, &msg, 0, s->id, 1,
                (const uint8_t *)"",
                (const uint8_t *)s->get_name().c_str(), 0, 0, 0, 0, 0, 0, 0);

            if (!_send_mavlink_message(&addr, msg)) {
                log_error("Sending camera information failed for camera %d.", s->id);
                return;
            }
            success = true;
        }
    }

    _send_ack(addr, command, success);
}

void MavlinkServer::_handle_mavlink_message(const struct sockaddr_in &addr, mavlink_message_t *msg)
{
    log_debug("Message received: (sysid: %d compid: %d msgid: %d)", msg->sysid, msg->compid,
              msg->msgid);

    if (msg->msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
        mavlink_command_long_t cmd;
        mavlink_msg_command_long_decode(msg, &cmd);

        if (cmd.target_system != _system_id || cmd.target_component != MAV_COMP_ID_CAMERA)
            return;

        switch (cmd.command) {
        case MAV_CMD_REQUEST_CAMERA_INFORMATION:
            this->_handle_camera_info_request(addr, cmd.command, cmd.param1 /* Camera ID */,
                                              cmd.param2 /* Action */);
            break;
        default:
            log_debug("Command %d unhandled. Discarding.", cmd.command);
        }
    }
}

void MavlinkServer::_message_received(const struct sockaddr_in &sockaddr, const struct buffer &buf)
{
    mavlink_message_t msg;
    mavlink_status_t status;

    for (unsigned int i = 0; i < buf.len; ++i) {
        //TOOD: Parse mavlink message all at once, instead of using mavlink_parse_char
        if (mavlink_parse_char(MAVLINK_COMM_0, buf.data[i], &msg, &status))
            _handle_mavlink_message(sockaddr, &msg);
    }
}

bool MavlinkServer::_send_mavlink_message(const struct sockaddr_in *addr, mavlink_message_t &msg)
{
    uint8_t buffer[MAX_MAVLINK_MESSAGE_SIZE];
    struct buffer buf = {0, buffer};

    buf.len = mavlink_msg_to_send_buffer(buf.data, &msg);

    if (addr)
        return buf.len > 0 && _udp.write(buf, *addr) > 0;
    return buf.len > 0 && _udp.write(buf, _broadcast_addr) > 0;
}

bool _heartbeat_cb(void *data)
{
    assert(data);
    MavlinkServer *server = (MavlinkServer *)data;
    mavlink_message_t msg;

    mavlink_msg_heartbeat_pack(server->_system_id, MAV_COMP_ID_CAMERA, &msg, MAV_TYPE_GENERIC,
                               MAV_AUTOPILOT_INVALID, MAV_MODE_PREFLIGHT, 0, MAV_STATE_ACTIVE);
    if (!server->_send_mavlink_message(nullptr, msg))
        log_error("Sending HEARTBEAT failed.");

    return true;
}

void MavlinkServer::start()
{
    if (_is_running)
        return;
    _is_running = true;

    _udp.open(true);
    _udp.set_read_callback([this](const struct buffer &buf, const struct sockaddr_in &sockaddr) {
        this->_message_received(sockaddr, buf);
    });
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
