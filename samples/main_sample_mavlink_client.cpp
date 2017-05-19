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
#include <unistd.h>

#include "glib_mainloop.h"
#include "log.h"
#include "socket.h"
#include "util.h"

#define BUF_LEN 1024
#define MAX_STREAMS 25
#define GCS_SYSID 255

struct stream {
    int id;
    const char *name;
};

struct Context {
    UDPSocket udp;
    int connected_camera_sysid = 0;
    struct stream streams[MAX_STREAMS];
    uint8_t streams_size;
};

static void reprint_streams(struct Context &ctx)
{
    log_info("\n");
    if (ctx.streams_size == 0) {
        log_info("No streams found.");
        return;
    }

    log_info("Streams found:");
    for (int i = 0; i < ctx.streams_size; i++) {
        log_info("%d - %s", ctx.streams[i].id, ctx.streams[i].name);
    }
}

static bool send_camera_msg(struct Context &ctx, const struct sockaddr_in &addr,
                            mavlink_message_t &msg)
{
    uint8_t buffer[BUF_LEN];
    struct buffer buf = {0, buffer};

    buf.len = mavlink_msg_to_send_buffer(buf.data, &msg);
    if (!buf.len || ctx.udp.write(buf, addr) < 0) {
        log_error("Sending camera request failed.");
        return false;
    }

    return true;
}

static int read_int()
{
    char *text = nullptr;
    int ret, value;
    ssize_t read;
    size_t n, len;

    do {
        read = getline(&text, &n, stdin);
        if (read <= 0) {
            log_error("Reading input failed. aborting");
            exit(EXIT_FAILURE);
            return 0;
        }

        len = strlen(text);
        if (len > 0) {
            text[len - 1] = 0;
            ret = safe_atoi(text, &value);
            if (ret >= 0) {
                free(text);
                return value;
            }
        }

        log_error("Invalid value. Try again:");
    } while (true);
}

static struct stream *get_camera_stream(struct Context &ctx, int camera_id)
{
    for (int i = 0; i < ctx.streams_size; i++) {
        if (ctx.streams[i].id == camera_id)
            return &ctx.streams[i];
    }

    return nullptr;
}

static void play_stream(struct Context &ctx, const struct sockaddr_in &addr)
{
    int camera_id;
    struct stream *s;

    log_info("Please make your selection (type stream number):");
    camera_id = read_int();
    s = get_camera_stream(ctx, camera_id);

    if (!s) {
        log_error("Camera not found.");
        return;
    }

    mavlink_message_t out_msg;
    mavlink_msg_command_long_pack(GCS_SYSID, MAV_COMP_ID_ALL, &out_msg, ctx.connected_camera_sysid,
                                  MAV_COMP_ID_CAMERA, MAV_CMD_REQUEST_VIDEO_STREAM_INFORMATION, 0,
                                  camera_id, 1, 0, 0, 0, 0, 0);

    send_camera_msg(ctx, addr, out_msg);
}

static void handle_mavlink_message(struct Context &ctx, const struct sockaddr_in &addr, mavlink_message_t &msg)
{
    if (msg.compid != MAV_COMP_ID_CAMERA)
        return;

    if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
        if (msg.sysid == ctx.connected_camera_sysid)
            return;

        log_info("Camera Daemon found: sysid: %d", msg.sysid);
        ctx.connected_camera_sysid = msg.sysid;

        mavlink_message_t out_msg;
        mavlink_msg_command_long_pack(GCS_SYSID, MAV_COMP_ID_ALL, &out_msg,
                                      ctx.connected_camera_sysid, MAV_COMP_ID_CAMERA,
                                      MAV_CMD_REQUEST_CAMERA_INFORMATION, 0, 0, 1, 0, 0, 0, 0, 0);

        send_camera_msg(ctx, addr, out_msg);
    } else if (msg.msgid == MAVLINK_MSG_ID_CAMERA_INFORMATION) {
        mavlink_camera_information_t info;
        mavlink_msg_camera_information_decode(&msg, &info);

        ctx.streams[ctx.streams_size].id = info.camera_id;
        ctx.streams[ctx.streams_size++].name = strdup((const char *)info.model_name);
    } else if (msg.msgid == MAVLINK_MSG_ID_VIDEO_STREAM_INFORMATION) {
        char cmd[300];
        int ret;
        mavlink_video_stream_information_t info;

        mavlink_msg_video_stream_information_decode(&msg, &info);

        log_info("Stream Information:");
        log_info("   Status: %s", info.status == 1 ? "streaming" : "not streaming");
        log_info("   URI: %s", info.uri);

        if (info.status == 1) {
            log_info("Video is already streaming to another client.");
            exit(EXIT_FAILURE);
        } else {
            ret = snprintf(cmd, sizeof(cmd),
                    "gst-launch-1.0 rtspsrc location=%s ! decodebin ! autovideosink sync=false",
                    info.uri);
            if (ret >= (int)sizeof(cmd) || system(cmd) <= 0) {
                log_error("Unable to start video stream. Start it manually.");
            }
        }
    } else if (msg.msgid == MAVLINK_MSG_ID_COMMAND_ACK) {
        mavlink_command_ack_t ack;
        mavlink_msg_command_ack_decode(&msg, &ack);
        if (ack.command == MAV_CMD_REQUEST_CAMERA_INFORMATION) {
            reprint_streams(ctx);
            play_stream(ctx, addr);
        }
    }
}

static void message_received(struct Context &ctx, const struct sockaddr_in &addr, const struct buffer &buf)
{
    mavlink_message_t msg;
    mavlink_status_t status;

    for (unsigned int i = 0; i < buf.len; ++i) {
        if (mavlink_parse_char(MAVLINK_COMM_0, buf.data[i], &msg, &status)) {
            handle_mavlink_message(ctx, addr, msg);
        }
    }
}

int main(int argc, char *argv[])
{
    Context ctx = {};
    GlibMainloop mainloop;

    Log::open();
    Log::set_max_level(Log::Level::INFO);
    log_debug("Camera Streaming MAVLink Client");

    ctx.udp.open(false);
    ctx.udp.bind("0.0.0.0", 14550);
    ctx.udp.set_read_callback([&ctx](const struct buffer &buf, const struct sockaddr_in &sockaddr) {
        message_received(ctx, sockaddr, buf);
    });

    mainloop.loop();

    Log::close();
    return 0;
}
