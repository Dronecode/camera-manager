/*
 * This file is part of the Camera Streaming Daemon project
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

#include "glib_mainloop.h"
#include "log.h"
#include "socket.h"
#include "util.h"

#define BUF_LEN 1024

struct Context {
    UDPSocket udp;
    int connected_camera_sysid = 0;
};

static bool send_camera_request(UDPSocket &udp)
{
    uint8_t buffer[BUF_LEN];
    struct buffer buf = {0, buffer};

    mavlink_message_t msg;
    mavlink_msg_command_long_pack(255, MAV_COMP_ID_ALL, &msg, 1, MAV_COMP_ID_CAMERA,
                                  MAV_CMD_REQUEST_CAMERA_INFORMATION, 0, 1, 0, 0, 0, 0, 0, 0);

    buf.len = mavlink_msg_to_send_buffer(buf.data, &msg);
    if (!buf.len || udp.write(buf) < 0) {
        log_error("Sending camera request failed.");
        return false;
    }

    return true;
}

static void handle_mavlink_message(struct Context &ctx, mavlink_message_t &msg)
{
    if (msg.compid != MAV_COMP_ID_CAMERA)
        return;

    if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
        if (msg.sysid == ctx.connected_camera_sysid)
            return;

        log_info("Camera Daemon found: sysid: %d", msg.sysid);
        ctx.connected_camera_sysid = msg.sysid;
        send_camera_request(ctx.udp);
    } else if (msg.msgid == MAVLINK_MSG_ID_CAMERA_INFORMATION) {
        mavlink_camera_information_t info;
        mavlink_msg_camera_information_decode(&msg, &info);
        log_info("Camera found: id: %d model: %s", info.camera_id, (const char *)info.model_name);
    }
}

static void message_received(struct Context &ctx, const struct buffer &buf)
{
    mavlink_message_t msg;
    mavlink_status_t status;

    for (unsigned int i = 0; i < buf.len; ++i) {
        if (mavlink_parse_char(MAVLINK_COMM_0, buf.data[i], &msg, &status)) {
            handle_mavlink_message(ctx, msg);
        }
    }
}

int main(int argc, char *argv[])
{
    Context ctx;
    GlibMainloop mainloop;

    Log::open();
    Log::set_max_level(Log::Level::DEBUG);
    log_debug("Camera Streaming MAVLink Client");

    ctx.udp.open("0.0.0.0", 14550, true);
    ctx.udp.set_read_callback([&ctx](const struct buffer &buf) { message_received(ctx, buf); });
    mainloop.loop();

    Log::close();
    return 0;
}
