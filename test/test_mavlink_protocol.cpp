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

/**

@brief  This is  a test application to test mavlink messages in the Camera Streaming Daemon.

*/

#include <assert.h>
#include <mavlink.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <unistd.h>

#include "glib_mainloop.h"
#include "log.h"
#include "socket.h"
#include "util.h"

#define BUF_LEN 1024
#define MAX_STREAMS 25
#define GCS_SYSID 255

using namespace std;

class Drone {
public:
    enum Mode { NONE = -1, IMAGE, VIDEO, SURVEY };
    Drone();
    bool setMode(int camera_id, int mode);
    int getMode(int camera_id);
    void imageCapture(int camera_id, int count, int interval);
    std::vector<int> getCameraIdList() const;
    std::string getCameraName(int camera_id) const;
    bool getCameraStream(int camera_id) const;

private:
    struct Stream {
        int id;
        std::string name;
        Mode mode;
    };
    UDPSocket udp;
    int sysid = 0;
    std::vector<Stream> streams;
    struct sockaddr_in addr;
    struct buffer buf;
    bool sendCameraMsg(mavlink_message_t &msg);
    void handleCameraInformationCB(mavlink_message_t &msg);
    void handleCameraSettingsCB(mavlink_message_t &msg);
    void handleHeartbeatCB(mavlink_message_t &msg);
    void handleAckCB(mavlink_message_t &msg);
    void handleMavlinkMessageCB(mavlink_message_t &msg);
    void messageReceivedCB();
};

Drone::Drone()
{
    udp.open(false);
    udp.bind("0.0.0.0", 14550);
    udp.set_read_callback([this](const struct buffer &buff, const struct sockaddr_in &sockaddr) {
        addr = sockaddr;
        buf = buff;
        messageReceivedCB();
    });
}

std::vector<int> Drone::getCameraIdList() const
{
    std::vector<int> camera_id_list;
    if (streams.size() == 0) {
        log_info("No streams found.");

    } else {
        for (std::vector<Stream>::const_iterator id_list = streams.begin();
             id_list != streams.end(); ++id_list) {
            camera_id_list.push_back(id_list->id);
        }
        return camera_id_list;
    }
}

std::string Drone::getCameraName(int camera_id) const
{

    for (auto camera : streams) {
        if (camera.id == camera_id)
            return camera.name;
    }
    return nullptr;
}

bool Drone::sendCameraMsg(mavlink_message_t &msg)
{
    uint8_t buffer[BUF_LEN];
    struct buffer buff = {0, buffer};

    buff.len = mavlink_msg_to_send_buffer(buff.data, &msg);
    if (!buff.len || udp.write(buff, addr) < 0) {
        log_error("Sending camera request failed.");
        return false;
    }

    return true;
}

bool Drone::getCameraStream(int camera_id) const
{

    for (auto camera : streams) {
        if (camera.id == camera_id)
            return 1;
    }
    return 0;
}

bool Drone::setMode(int camera_id, int mode)
{
    bool success;
    mavlink_message_t out_msg;
    if (mode == IMAGE) {
        mavlink_msg_command_long_pack(GCS_SYSID, MAV_COMP_ID_ALL, &out_msg, sysid, camera_id,
                                      MAV_CMD_SET_CAMERA_MODE, 0, 0, 0, 0, 0, 0, 0, 0);
        log_info("MAV_CMD_SET_CAMERA_MODE sent");
        sendCameraMsg(out_msg);
        success = 1;
    } else if (mode == VIDEO) {
        mavlink_msg_command_long_pack(GCS_SYSID, MAV_COMP_ID_ALL, &out_msg, sysid, camera_id,
                                      MAV_CMD_SET_CAMERA_MODE, 0, 0, 1, 0, 0, 0, 0, 0);
        log_info("MAV_CMD_SET_CAMERA_MODE sent");
        sendCameraMsg(out_msg);
        success = 1;
    } else {
        log_info("Mode value is invalid.");
        success = 0;
    }
    return success;
}

int Drone::getMode(int camera_id)
{
    mavlink_message_t out_msg;
    mavlink_msg_command_long_pack(GCS_SYSID, MAV_COMP_ID_ALL, &out_msg, sysid, camera_id,
                                  MAV_CMD_REQUEST_CAMERA_SETTINGS, 0, 1, 0, 0, 0, 0, 0, 0);
    log_info("MAV_CMD_REQUEST_CAMERA_SETTINGS sent");
    sendCameraMsg(out_msg);

    for (auto &i : streams) {
        if (i.id == camera_id) {
            while (i.mode == NONE) {
                sleep(1);
            }
            return i.mode;
        }
    }
}

void Drone::imageCapture(int camera_id, int count, int interval)
{
    mavlink_message_t out_msg;
    mavlink_msg_command_long_pack(GCS_SYSID, MAV_COMP_ID_ALL, &out_msg, sysid, camera_id,
                                  MAV_CMD_IMAGE_START_CAPTURE, 0, interval, count, 1, 0, 0, 0, 0);
    log_info("MAV_CMD_IMAGE_START_CAPTURE sent");
    sendCameraMsg(out_msg);
}

void Drone::handleHeartbeatCB(mavlink_message_t &msg)
{
    if (msg.sysid == sysid) {
        for (auto i : streams)
            if (msg.compid == i.id) {
                return;
            }
    }

    log_info("Camera Daemon found: sysid: %d comp_id: %d", msg.sysid, msg.compid);
    sysid = msg.sysid;

    mavlink_message_t out_msg;
    mavlink_msg_command_long_pack(GCS_SYSID, MAV_COMP_ID_ALL, &out_msg, sysid, msg.compid,
                                  MAV_CMD_REQUEST_CAMERA_INFORMATION, 0, 1, 0, 0, 0, 0, 0, 0);
    log_info("MAV_CMD_REQUEST_CAMERA_INFORMATION sent");
    sendCameraMsg(out_msg);
}

void Drone::handleCameraInformationCB(mavlink_message_t &msg)
{
    mavlink_camera_information_t info;
    mavlink_msg_camera_information_decode(&msg, &info);
    struct Stream s;
    s.mode = NONE;
    s.id = msg.compid;
    s.name = std::string((char *)info.model_name);
    streams.push_back(s);
}

void Drone::handleCameraSettingsCB(mavlink_message_t &msg)
{
    log_info("Got MAVLINK_MSG_ID_CAMERA_SETTINGS");
    mavlink_camera_settings_t settings;
    mavlink_msg_camera_settings_decode(&msg, &settings);

    for (auto &i : streams) {
        if (i.id == msg.compid) {
            i.mode = (Mode)settings.mode_id;
        }
    }
}

void Drone::handleAckCB(mavlink_message_t &msg)
{
    mavlink_command_ack_t ack;
    mavlink_msg_command_ack_decode(&msg, &ack);
    if (ack.command == MAV_CMD_IMAGE_START_CAPTURE) {
        log_info("Acknowledgement for MAV_CMD_IMAGE_START_CAPTURE recieved");
        return;
    }
}

void Drone::handleMavlinkMessageCB(mavlink_message_t &msg)
{

    if (msg.compid < MAV_COMP_ID_CAMERA2) {
        return;
    }

    switch (msg.msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT:
        handleHeartbeatCB(msg);
        break;
    case MAVLINK_MSG_ID_CAMERA_INFORMATION:
        handleCameraInformationCB(msg);
        break;
    case MAVLINK_MSG_ID_CAMERA_SETTINGS:
        handleCameraSettingsCB(msg);
        break;
    case MAVLINK_MSG_ID_COMMAND_ACK:
        handleAckCB(msg);
        break;
    default:
        log_info("%d  message is not handled.", msg.msgid);
    }
}

void Drone::messageReceivedCB()
{
    mavlink_message_t msg;
    mavlink_status_t status;

    for (unsigned int i = 0; i < buf.len; ++i) {
        if (mavlink_parse_char(MAVLINK_COMM_0, buf.data[i], &msg, &status)) {
            handleMavlinkMessageCB(msg);
        }
    }
}
// Thread to handle mavlink messages
void *discovercam(void *cntx)
{
    GlibMainloop mainloop;

    class Drone **ctx = (class Drone **)cntx;
    *ctx = new Drone();

    mainloop.loop();
}
int main(int argc, char *argv[])
{
    int camera_id;
    Log::open();
    Log::set_max_level(Log::Level::INFO);
    log_debug("Camera Streaming MAVLink Client");

    class Drone *ctx;
    std::thread t_id;
    t_id = std::thread(discovercam, (void *)&ctx);
    sleep(5);
    auto camera_list = ctx->getCameraIdList();
    log_info("\n");
    if (camera_list.size() == 0) {
        log_info("No streams found.");
        exit(EXIT_FAILURE);
    }
    for (auto camera : camera_list) {
        log_info("%d-%s", camera, (ctx->getCameraName(camera)).c_str());
    }

    log_info("Please make your selection (type stream number):");
    scanf("%d", &camera_id);
    if (!ctx->getCameraStream(camera_id)) {
        log_error("Camera not found.");
        exit(EXIT_FAILURE);
        ;
    }

    do {
        log_info("\nSelect an action\n 1.Set Mode\n 2.Get Mode\n 3.Image Capture\n 4.Exit");
        int option;
        scanf("%d", &option);
        switch (option) {
        case 1: {
            log_info("Select mode 0:image 1:video 2:image survey");
            int mode;
            scanf("%d", &mode);
            ctx->setMode(camera_id, mode);
            break;
        }
        case 2: {
            int mode = ctx->getMode(camera_id);
            sleep(1);
            switch (mode) {
            case Drone::IMAGE:
                log_info("Mode: image");
                break;
            case Drone::VIDEO:
                log_info("Mode: video");
                break;
            case Drone::SURVEY:
                log_info("Mode: image survey");
                break;
            default:
                log_info("Mode: Invalid");
            }
            break;
        }
        case 3: {
            log_info("Enter the number of images to be taken");
            int count;
            scanf("%d", &count);
            log_info("Enter the intervel in which images to be taken");
            int intervel;
            scanf("%d", &intervel);
            ctx->imageCapture(camera_id, count, intervel);
            sleep(intervel + 1);
            break;
        }
        case 4:
            log_info("Exiting appication");
            break;
        default:
            log_info("Invalid Selection");
        }
        if (option == 4)
            exit(EXIT_FAILURE);
    } while (1);

    t_id.join();

    Log::close();
    return 0;
}
