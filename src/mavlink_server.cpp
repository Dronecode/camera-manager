/*
 * This file is part of the Dronecode Camera Manager
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
#include <cmath>
#include <mavlink.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"
#include "mainloop.h"
#include "mavlink_server.h"
#include "util.h"

using namespace std::placeholders;

#define DEFAULT_MAVLINK_PORT 14550
#define DEFAULT_MAVLINK_BROADCAST_ADDR "255.255.255.255"
#define DEFAULT_RTSP_SERVER_ADDR "0.0.0.0"
#define MAX_MAVLINK_MESSAGE_SIZE 1024
#define DEFAULT_SYSTEM_ID 1

static const float epsilon = std::numeric_limits<float>::epsilon();

MavlinkServer::MavlinkServer(const ConfFile &conf, std::vector<std::unique_ptr<Stream>> &streams,
                             RTSPServer &rtsp)
    : _streams(streams)
    , _is_running(false)
    , _timeout_handler(0)
    , _broadcast_addr{}
    , _is_sys_id_found(false)
    , _system_id(DEFAULT_SYSTEM_ID)
    , _comp_id(MAV_COMP_ID_CAMERA)
    , _rtsp_server_addr(nullptr)
    , _rtsp(rtsp)
{
    struct options {
        unsigned long int port;
        int sysid;
        int compid;
        char *rtsp_server_addr;
        char broadcast[17];
    } opt = {};
    static const ConfFile::OptionsTable option_table[] = {
        {"port", false, ConfFile::parse_ul, OPTIONS_TABLE_STRUCT_FIELD(options, port)},
        {"system_id", false, ConfFile::parse_i, OPTIONS_TABLE_STRUCT_FIELD(options, sysid)},
        {"rtsp_server_addr", false, ConfFile::parse_str_dup, OPTIONS_TABLE_STRUCT_FIELD(options, rtsp_server_addr)},
        {"broadcast_addr", false, ConfFile::parse_str_buf, OPTIONS_TABLE_STRUCT_FIELD(options, broadcast)},
    };
    conf.extract_options("mavlink", option_table, ARRAY_SIZE(option_table), (void *)&opt);

    if (opt.port)
        _broadcast_addr.sin_port = htons(opt.port);
    else
        _broadcast_addr.sin_port = htons(DEFAULT_MAVLINK_PORT);

    if (opt.sysid) {
        if (opt.sysid > 0 && opt.sysid < 255) {
            log_info("Use System ID %d, ignore heartbeat from Vehicle", opt.sysid);
            _system_id = opt.sysid;
            _is_sys_id_found = true;

        } else {
            log_error("Invalid System ID for MAVLink communication (%d)", opt.sysid);
            log_info("Use System ID %d, till heartbeat received from Vehicle", DEFAULT_SYSTEM_ID);
            _system_id = DEFAULT_SYSTEM_ID;
        }
    } else {
        log_info("Use System ID %d, till heartbeat received from Vehicle", DEFAULT_SYSTEM_ID);
    }

    if (opt.broadcast[0])
        _broadcast_addr.sin_addr.s_addr = inet_addr(opt.broadcast);
    else
        _broadcast_addr.sin_addr.s_addr = inet_addr(DEFAULT_MAVLINK_BROADCAST_ADDR);
    _broadcast_addr.sin_family = AF_INET;
    _rtsp_server_addr = opt.rtsp_server_addr;
}

MavlinkServer::~MavlinkServer()
{
    stop();
    free(_rtsp_server_addr);
}

void MavlinkServer::_send_ack(const struct sockaddr_in &addr, int cmd, int comp_id, bool success)
{
    mavlink_message_t msg;

    mavlink_msg_command_ack_pack(_system_id, comp_id, &msg, cmd,
                                 success ? MAV_RESULT_ACCEPTED : MAV_RESULT_FAILED, 255);

    if (!_send_mavlink_message(&addr, msg)) {
        log_error("Sending ack failed.");
        return;
    }
}

void MavlinkServer::_handle_request_camera_information(const struct sockaddr_in &addr,
                                                       mavlink_command_long_t &cmd)
{
    log_debug("%s", __func__);

    mavlink_message_t msg;
    bool success = false;

    // Take no action if flag not set
    if (std::abs(cmd.param1) <= epsilon) {
        log_warning("No Action");
        _send_ack(addr, cmd.command, cmd.target_component, true);
        return;
    }

    CameraComponent *tgtComp = getCameraComponent(cmd.target_component);
    if (tgtComp) {
        const CameraInfo &camInfo = tgtComp->getCameraInfo();
        mavlink_msg_camera_information_pack(
            _system_id, cmd.target_component, &msg, 0, (const uint8_t *)camInfo.vendorName,
            (const uint8_t *)camInfo.modelName, camInfo.firmware_version, camInfo.focal_length,
            camInfo.sensor_size_h, camInfo.sensor_size_v, camInfo.resolution_h,
            camInfo.resolution_v, camInfo.lens_id, camInfo.flags, camInfo.cam_definition_version,
            (const char *)camInfo.cam_definition_uri);

        if (!_send_mavlink_message(&addr, msg)) {
            log_error("Sending camera information failed for camera %d.", cmd.target_component);
            return;
        }

        success = true;
    }

    _send_ack(addr, cmd.command, cmd.target_component, success);
}

void MavlinkServer::_handle_request_camera_settings(const struct sockaddr_in &addr,
                                                    mavlink_command_long_t &cmd)
{
    log_debug("%s", __func__);

    // Take no action if flag not set
    if (std::abs(cmd.param1) <= epsilon) {
        log_warning("No Action");
        _send_ack(addr, cmd.command, cmd.target_component, true);
        return;
    }

    mavlink_message_t msg;
    bool success = false;

    CameraComponent *tgtComp = getCameraComponent(cmd.target_component);
    if (tgtComp) {
        // TODO:: Fill with appropriate mode value
        mavlink_msg_camera_settings_pack(_system_id, cmd.target_component, &msg, 0,
                                         CAMERA_MODE_IMAGE);

        if (!_send_mavlink_message(&addr, msg)) {
            log_error("Sending camera setting failed for camera %d.", cmd.target_component);
            return;
        }

        success = true;
    }

    _send_ack(addr, cmd.command, cmd.target_component, success);
}

void MavlinkServer::_handle_request_storage_information(const struct sockaddr_in &addr,
                                                        mavlink_command_long_t &cmd)
{
    log_debug("%s", __func__);

    // Take no action if flag not set
    if (std::abs(cmd.param2) <= epsilon) {
        log_warning("No Action");
        _send_ack(addr, cmd.command, cmd.target_component, true);
        return;
    }

    mavlink_message_t msg;
    bool success = false;

    CameraComponent *tgtComp = getCameraComponent(cmd.target_component);
    if (tgtComp) {
        // TODO:: Fill with appropriate value
        const StorageInfo &storeInfo = tgtComp->getStorageInfo();
        mavlink_msg_storage_information_pack(_system_id, cmd.target_component, &msg, 0,
                                             storeInfo.storage_id, storeInfo.storage_count,
                                             storeInfo.status, storeInfo.total_capacity,
                                             storeInfo.used_capacity, storeInfo.available_capacity,
                                             storeInfo.read_speed, storeInfo.write_speed);

        if (!_send_mavlink_message(&addr, msg)) {
            log_error("Sending storage information failed for camera %d.", cmd.target_component);
            return;
        }

        success = true;
    }

    _send_ack(addr, cmd.command, cmd.target_component, success);
}

void MavlinkServer::_handle_set_camera_mode(const struct sockaddr_in &addr,
                                                        mavlink_command_long_t &cmd)
{
    log_debug("%s", __func__);

    bool success = false;

    CameraComponent *tgtComp = getCameraComponent(cmd.target_component);
    if (tgtComp) {
        if (!tgtComp->setCameraMode(translateCameraMode((uint32_t)cmd.param2)))
            success = true;
    }

    _send_ack(addr, cmd.command, cmd.target_component, success);

}

void MavlinkServer::_handle_image_start_capture(const struct sockaddr_in &addr,
                                                mavlink_command_long_t &cmd)
{
    log_debug("%s", __func__);
    bool success = false;
    image_callback_t cb_data;

    CameraComponent *tgtComp = getCameraComponent(cmd.target_component);
    if (tgtComp) {
        cb_data.comp_id = cmd.target_component;
        cb_data.addr = addr;
        if (!tgtComp->startImageCapture(
                (uint32_t)cmd.param2 /*interval*/, (uint32_t)cmd.param3 /*count*/,
                std::bind(&MavlinkServer::_image_captured_cb, this, cb_data, _1, _2)))
            success = true;
    }

    _send_ack(addr, cmd.command, cmd.target_component, success);
}

void MavlinkServer::_handle_image_stop_capture(const struct sockaddr_in &addr,
                                               mavlink_command_long_t &cmd)
{
    log_debug("%s", __func__);

    bool success = false;

    CameraComponent *tgtComp = getCameraComponent(cmd.target_component);
    if (tgtComp) {
        if (!tgtComp->stopImageCapture())
            success = true;
    }

    _send_ack(addr, cmd.command, cmd.target_component, success);
}

void MavlinkServer::_image_captured_cb(image_callback_t cb_data, int result, int seq_num)
{
    log_debug("%s result:%d seq:%d", __func__, result, seq_num);
    log_debug("Comp Id:%d", cb_data.comp_id);
    // TODO :: Fill MAVLINK message with correct info including geo location etc
    bool success = !result;
    mavlink_message_t msg;
    float q[4] = {0}; // Quaternion of camera orientation
    mavlink_msg_camera_image_captured_pack(
        _system_id, cb_data.comp_id, &msg, 0 /*time_boot_ms*/, 0 /*time_utc*/, 1 /*camera_id*/,
        0 /*lat*/, 0 /*lon*/, 0 /*alt*/, 0 /*relative_alt*/, q, seq_num /*image_index*/,
        success /*capture_result*/, 0 /*file_url*/);

    if (!_send_mavlink_message(&cb_data.addr, msg)) {
        log_error("Sending camera image captured failed for camera %d.", cb_data.comp_id);
        return;
    }

    _send_camera_capture_status(cb_data.comp_id, cb_data.addr);
}

void MavlinkServer::_handle_video_start_capture(const struct sockaddr_in &addr,
                                                mavlink_command_long_t &cmd)
{
    log_debug("%s", __func__);
    bool success = false;
    image_callback_t cb_data;

    CameraComponent *tgtComp = getCameraComponent(cmd.target_component);
    if (tgtComp) {
        cb_data.comp_id = cmd.target_component;
        memcpy(&cb_data.addr, &addr, sizeof(struct sockaddr_in));
        if (!tgtComp->startVideoCapture((uint32_t)cmd.param2 /*camera_Capture_status freq*/))
            success = true;
    }

    _send_ack(addr, cmd.command, cmd.target_component, success);
}

void MavlinkServer::_handle_video_stop_capture(const struct sockaddr_in &addr,
                                               mavlink_command_long_t &cmd)
{
    log_debug("%s", __func__);

    bool success = false;

    CameraComponent *tgtComp = getCameraComponent(cmd.target_component);
    if (tgtComp) {
        if (!tgtComp->stopVideoCapture())
            success = true;
    }

    _send_ack(addr, cmd.command, cmd.target_component, success);
}

void MavlinkServer::_handle_request_camera_capture_status(const struct sockaddr_in &addr,
                                                          mavlink_command_long_t &cmd)
{
    log_debug("%s", __func__);

    // Take no action if flag not set
    if (std::abs(cmd.param1) <= epsilon) {
        log_warning("No Action");
        _send_ack(addr, cmd.command, cmd.target_component, true);
        return;
    }

    bool success = _send_camera_capture_status(cmd.target_component, addr);

    _send_ack(addr, cmd.command, cmd.target_component, success);
}

void MavlinkServer::_handle_camera_video_stream_request(const struct sockaddr_in &addr, int command,
                                                        unsigned int camera_id, unsigned int action)
{
    log_debug("%s", __func__);

    mavlink_message_t msg;
    char query[35] = "";

    if (action != 1)
        return;

    for (auto const &s : _streams) {
        if (camera_id == 0 || camera_id == s->id) {
            const Stream::FrameSize *fs = s->sel_frame_size
                ? s->sel_frame_size
                : _find_best_frame_size(*s, UINT32_MAX, UINT32_MAX);

            if (s->sel_frame_size) {
                int ret = snprintf(query, sizeof(query), "?width=%d&height=%d",
                                   s->sel_frame_size->width, s->sel_frame_size->height);
                if (ret > (int)sizeof(query)) {
                    log_error("Invalid requested resolution. Aborting request.");
                    return;
                }
            }

            mavlink_msg_video_stream_information_pack(
                _system_id, _comp_id, &msg, s->id, s->is_streaming /* Status */,
                0 /* FPS */, fs->width, fs->height, 0 /* bitrate */, 0 /* Rotation */,
                _rtsp.get_rtsp_uri(_rtsp_server_addr, *s, query).c_str());
            if (!_send_mavlink_message(&addr, msg)) {
                log_error("Sending camera information failed for camera %d.", s->id);
                return;
            }
        }
    }
}

const Stream::FrameSize *MavlinkServer::_find_best_frame_size(Stream &s, uint32_t w, uint32_t h)
{
    // Using strategy of getting the higher frame size that is lower than WxH, if the
    // exact resolution is not found
    const Stream::FrameSize *best = nullptr;
    for (auto const &f : s.formats) {
        for (auto const &fs : f.frame_sizes) {
            if (fs.width == w && fs.height == h)
                return &fs;
            else if (!best || (fs.width <= w && fs.width >= best->width && fs.height <= h
                               && fs.height >= best->height))
                best = &fs;
        }
    }
    return best;
}

void MavlinkServer::_handle_camera_set_video_stream_settings(const struct sockaddr_in &addr,
                                                             mavlink_message_t *msg)
{
    log_debug("%s", __func__);

    mavlink_set_video_stream_settings_t settings;
    Stream *stream = nullptr;

    mavlink_msg_set_video_stream_settings_decode(msg, &settings);
    for (auto const &s : _streams) {
        if (s->id == settings.camera_id) {
            stream = &*s;
        }
    }
    if (!stream) {
        log_debug("SET_VIDEO_STREAM request in an invalid camera (camera_id = %d)",
                  settings.camera_id);
        return;
    }

    if (settings.resolution_h == 0 || settings.resolution_v == 0)
        stream->sel_frame_size = nullptr;
    else
        stream->sel_frame_size
            = _find_best_frame_size(*stream, settings.resolution_h, settings.resolution_v);
}

void MavlinkServer::_handle_param_ext_request_read(const struct sockaddr_in &addr,
                                                   mavlink_message_t *msg)
{
    log_debug("%s", __func__);

    mavlink_message_t msg2;
    mavlink_param_ext_request_read_t param_ext_read;
    mavlink_param_ext_value_t param_ext_value;
    bool ret = false;
    mavlink_msg_param_ext_request_read_decode(msg, &param_ext_read);
    CameraComponent *tgtComp = getCameraComponent(param_ext_read.target_component);
    if (tgtComp) {
        // Null terminate param_id
        // Read parameter value from camera component
        ret = tgtComp->getParam(param_ext_read.param_id, sizeof(param_ext_read.param_id),
                                param_ext_value.param_value, sizeof(param_ext_value.param_value));
        if (!ret) {
            // Send the param value to GCS
            param_ext_value.param_count = 1;
            param_ext_value.param_index = 0;
            // Copy the param id from req msg to resp msg
            mem_cpy(param_ext_value.param_id, sizeof(param_ext_value.param_id),
                    param_ext_read.param_id, sizeof(param_ext_read.param_id),
                    sizeof(param_ext_value.param_id));
            param_ext_value.param_type
                = tgtComp->getParamType(param_ext_value.param_id, sizeof(param_ext_value.param_id));
            mavlink_msg_param_ext_value_encode(_system_id, param_ext_read.target_component, &msg2,
                                               &param_ext_value);
        } else {
            // Send param ack error to GCS
            mavlink_param_ext_ack_t param_ext_ack;
            // Copy the param id from req msg to resp msg
            mem_cpy(param_ext_ack.param_id, sizeof(param_ext_ack.param_id), param_ext_read.param_id,
                    sizeof(param_ext_read.param_id), sizeof(param_ext_ack.param_id));
            param_ext_ack.param_type
                = tgtComp->getParamType(param_ext_value.param_id, sizeof(param_ext_value.param_id));
            param_ext_ack.param_result = PARAM_ACK_FAILED;
            mavlink_msg_param_ext_ack_encode(_system_id, param_ext_read.target_component, &msg2,
                                             &param_ext_ack);
        }
        if (!_send_mavlink_message(&addr, msg2)) {
            log_error("Sending response to param request read failed %d.",
                      param_ext_read.target_component);
            return;
        }
    }
}
void MavlinkServer::_handle_param_ext_request_list(const struct sockaddr_in &addr,
                                                   mavlink_message_t *msg)
{
    log_debug("%s", __func__);

    int idx = 0;
    mavlink_message_t msg2;
    mavlink_param_ext_request_list_t param_list;
    mavlink_param_ext_value_t param_ext_value;
    mavlink_msg_param_ext_request_list_decode(msg, &param_list);
    CameraComponent *tgtComp = getCameraComponent(param_list.target_component);
    if (tgtComp) {
        // Get the list of parameter from camera component
        const std::map<std::string, std::string> &paramIdtoValue = tgtComp->getParamList();
        param_ext_value.param_count = paramIdtoValue.size();

        // Send each param,value to GCS
        for (auto &x : paramIdtoValue) {
            param_ext_value.param_index = idx++;
            // Copy the param id
            mem_cpy(param_ext_value.param_id, sizeof(param_ext_value.param_id), x.first.c_str(),
                    x.first.size() + 1, sizeof(param_ext_value.param_id));
            // Copy the param value
            mem_cpy(param_ext_value.param_value, sizeof(param_ext_value.param_value),
                    x.second.data(), x.second.size(), sizeof(param_ext_value.param_id));
            param_ext_value.param_type = tgtComp->getParamType(x.first.c_str(), x.first.size());
            mavlink_msg_param_ext_value_encode(_system_id, param_list.target_component, &msg2,
                                               &param_ext_value);
            if (!_send_mavlink_message(&addr, msg2)) {
                log_error("Sending response to param request list failed %d.", idx);
            }
        }
    }
}

void MavlinkServer::_handle_param_ext_set(const struct sockaddr_in &addr, mavlink_message_t *msg)
{
    log_debug("%s", __func__);

    bool ret = false;
    mavlink_message_t msg2;
    mavlink_param_ext_set_t param_set;
    mavlink_param_ext_ack_t param_ext_ack;
    mavlink_msg_param_ext_set_decode(msg, &param_set);
    CameraComponent *tgtComp = getCameraComponent(param_set.target_component);
    if (tgtComp) {
        // Set parameter
        // TODO:: Ensure that param_id is null terminated before passing
        ret = tgtComp->setParam(param_set.param_id, sizeof(param_set.param_id),
                                param_set.param_value, sizeof(param_set.param_value),
                                param_set.param_type);
        // Copy id from req msg to response msg
        mem_cpy(param_ext_ack.param_id, sizeof(param_ext_ack.param_id), param_set.param_id,
                sizeof(param_set.param_id), sizeof(param_ext_ack.param_id));
        param_ext_ack.param_type = param_set.param_type;
        if (!ret) {
            // Send response to GCS
            mem_cpy(param_ext_ack.param_value, sizeof(param_ext_ack.param_value),
                    param_set.param_value, sizeof(param_set.param_value),
                    sizeof(param_ext_ack.param_value));
            param_ext_ack.param_result = PARAM_ACK_ACCEPTED;
        } else {
            // Send error alongwith current value of the param to GCS
            tgtComp->getParam(param_ext_ack.param_id, sizeof(param_ext_ack.param_id),
                              param_ext_ack.param_value, sizeof(param_ext_ack.param_value));
            param_ext_ack.param_result = PARAM_ACK_FAILED;
        }

        mavlink_msg_param_ext_ack_encode(_system_id, param_set.target_component, &msg2,
                                         &param_ext_ack);
        if (!_send_mavlink_message(&addr, msg2)) {
            log_error("Sending response to param set failed %d.", param_set.target_component);
            return;
        }
    }
}

void MavlinkServer::_handle_reset_camera_settings(const struct sockaddr_in &addr,
                                                  mavlink_command_long_t &cmd)
{
    log_debug("%s", __func__);

    // Take no action if flag not set
    if (std::abs(cmd.param1) <= epsilon) {
        log_warning("No Action");
        _send_ack(addr, cmd.command, cmd.target_component, true);
        return;
    }

    bool success = false;
    CameraComponent *tgtComp = getCameraComponent(cmd.target_component);

    if (tgtComp) {
        if (!tgtComp->resetCameraSettings())
            success = true;
    }

    _send_ack(addr, cmd.command, cmd.target_component, success);
}
void MavlinkServer::_handle_heartbeat(const struct sockaddr_in &addr, mavlink_message_t *msg)
{
    mavlink_heartbeat_t heartbeat;
    mavlink_msg_heartbeat_decode(msg, &heartbeat);

    if (heartbeat.autopilot == MAV_AUTOPILOT_PX4) {
        if (msg->sysid > 0 && msg->sysid < 255) {
            log_info("Heartbeat received, System ID = %d", msg->sysid);
            _system_id = msg->sysid;
            _is_sys_id_found = true;
        }
    }
}

void MavlinkServer::_handle_mavlink_message(const struct sockaddr_in &addr, mavlink_message_t *msg)
{
    // log_debug("Message received: (sysid: %d compid: %d msgid: %d)", msg->sysid, msg->compid,
    //          msg->msgid);

    if (msg->msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
        mavlink_command_long_t cmd;
        mavlink_msg_command_long_decode(msg, &cmd);
        log_debug("Command received: (sysid: %d compid: %d msgid: %d)", cmd.target_system,
                  cmd.target_component, cmd.command);

        if (cmd.target_system != _system_id || cmd.target_component < MAV_COMP_ID_CAMERA
            || cmd.target_component > MAV_COMP_ID_CAMERA6)
            return;

        if (compIdToObj.find(cmd.target_component) == compIdToObj.end())
            return;

        switch (cmd.command) {
        case MAV_CMD_REQUEST_CAMERA_INFORMATION:
            this->_handle_request_camera_information(addr, cmd);
            break;
        case MAV_CMD_REQUEST_VIDEO_STREAM_INFORMATION:
            this->_handle_camera_video_stream_request(addr, cmd.command, cmd.param1 /* Camera ID */,
                                                      cmd.param2 /* Action */);
            break;
        case MAV_CMD_REQUEST_CAMERA_SETTINGS:
            this->_handle_request_camera_settings(addr, cmd);
            break;
        case MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS:
            this->_handle_request_camera_capture_status(addr, cmd);
            break;
        case MAV_CMD_RESET_CAMERA_SETTINGS:
            this->_handle_reset_camera_settings(addr, cmd);
            break;
        case MAV_CMD_REQUEST_STORAGE_INFORMATION:
            this->_handle_request_storage_information(addr, cmd);
            break;
        case MAV_CMD_STORAGE_FORMAT:
            log_debug("MAV_CMD_STORAGE_FORMAT");
            break;
        case MAV_CMD_SET_CAMERA_MODE:
            this->_handle_set_camera_mode(addr, cmd);
            break;
        case MAV_CMD_IMAGE_START_CAPTURE:
            log_debug("MAV_CMD_IMAGE_START_CAPTURE");
            this->_handle_image_start_capture(addr, cmd);
            break;
        case MAV_CMD_IMAGE_STOP_CAPTURE:
            log_debug("MAV_CMD_IMAGE_STOP_CAPTURE");
            this->_handle_image_stop_capture(addr, cmd);
            break;
        case MAV_CMD_VIDEO_START_CAPTURE:
            log_debug("MAV_CMD_VIDEO_START_CAPTURE");
            this->_handle_video_start_capture(addr, cmd);
            break;
        case MAV_CMD_VIDEO_STOP_CAPTURE:
            log_debug("MAV_CMD_VIDEO_STOP_CAPTURE");
            this->_handle_video_stop_capture(addr, cmd);
            break;
        case MAV_CMD_REQUEST_CAMERA_IMAGE_CAPTURE:
        case MAV_CMD_DO_TRIGGER_CONTROL:
        case MAV_CMD_VIDEO_START_STREAMING:
        case MAV_CMD_VIDEO_STOP_STREAMING:
        default:
            log_debug("Command %d unhandled. Discarding.", cmd.command);
            break;
        }
    } else {
        switch (msg->msgid) {
        case MAVLINK_MSG_ID_HEARTBEAT:
            if (!_is_sys_id_found)
                this->_handle_heartbeat(addr, msg);
            break;
        case MAVLINK_MSG_ID_SET_VIDEO_STREAM_SETTINGS:
            this->_handle_camera_set_video_stream_settings(addr, msg);
            break;
        case MAVLINK_MSG_ID_PARAM_EXT_REQUEST_READ:
            this->_handle_param_ext_request_read(addr, msg);
            break;
        case MAVLINK_MSG_ID_PARAM_EXT_REQUEST_LIST:
            this->_handle_param_ext_request_list(addr, msg);
            break;
        case MAVLINK_MSG_ID_PARAM_EXT_SET:
            this->_handle_param_ext_set(addr, msg);
            break;
        default:
            // log_debug("Message %d unhandled, Discarding", msg->msgid);
            break;
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

bool MavlinkServer::_send_camera_capture_status(int compid, const struct sockaddr_in &addr)
{
    log_debug("%s", __func__);

    bool success = false;
    mavlink_message_t msg;
    uint32_t time_boot_ms = 0;
    uint8_t image_status = 0;
    uint8_t video_status = 0;
    int image_interval = 0;
    uint32_t recording_time_ms = 0;
    int available_capacity = 50; // in MiB
    CameraComponent *tgtComp = getCameraComponent(compid);
    if (tgtComp) {
        // Get image capture status
        tgtComp->getImageCaptureStatus(image_status, image_interval);
        // Get video capture status
        video_status = tgtComp->getVideoCaptureStatus();
        mavlink_msg_camera_capture_status_pack(_system_id, compid, &msg, time_boot_ms, image_status,
                                               video_status, static_cast<float>(image_interval),
                                               recording_time_ms,
                                               static_cast<float>(available_capacity));
        if (!_send_mavlink_message(&addr, msg)) {
            log_error("Sending camera setting failed for camera %d.", compid);
            return false;
        }

        success = true;
    }

    return success;
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

    if (server->_system_id < 1 || server->_system_id >= 255) {
        log_error("Invalid system_id. Heartbeat not sent.");
        return false;
    }

    for (std::map<int, CameraComponent *>::iterator it = server->compIdToObj.begin();
         it != server->compIdToObj.end(); it++) {
        /* log_debug("Sending heartbeat for component :%d system_id:%d", it->first,
                  server->_system_id);*/
        mavlink_msg_heartbeat_pack(server->_system_id, it->first, &msg, MAV_TYPE_GENERIC,
                                   MAV_AUTOPILOT_INVALID, MAV_MODE_PREFLIGHT, 0, MAV_STATE_ACTIVE);
        if (!server->_send_mavlink_message(nullptr, msg))
            log_error("Sending HEARTBEAT failed.");
    }
    return true;
}

void MavlinkServer::start()
{
    log_error("MAVLINK START\n");
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

int MavlinkServer::addCameraComponent(CameraComponent *camComp)
{
    log_debug("%s", __func__);
    int ret = -1;
    int compid;
#ifdef ENABLE_GAZEBO
    // PX4-SITL Gazebo is running a camera component with id MAV_COMP_ID_CAMERA
    compid = MAV_COMP_ID_CAMERA2;
#else
    compid = MAV_COMP_ID_CAMERA;
#endif

    while (compid <= MAV_COMP_ID_CAMERA6) {
        if (compIdToObj.find(compid) == compIdToObj.end()) {
            compIdToObj.insert(std::make_pair(compid, camComp));
            ret = compid;
            break;
        }
        compid++;
    }

    return ret;
}

void MavlinkServer::removeCameraComponent(CameraComponent *camComp)
{
    log_debug("%s", __func__);

    if (!camComp)
        return;

    for (std::map<int, CameraComponent *>::iterator it = compIdToObj.begin();
         it != compIdToObj.end(); it++) {
        if ((it->second) == camComp) {
            compIdToObj.erase(it);
            break;
        }
    }

    return;
}

CameraComponent *MavlinkServer::getCameraComponent(int compID)
{
    if (compID < MAV_COMP_ID_CAMERA || compID > MAV_COMP_ID_CAMERA6)
        return NULL;

    std::map<int, CameraComponent *>::iterator it = compIdToObj.find(compID);
    if (it != compIdToObj.end())
        return it->second;
    else
        return NULL;
}

CameraParameters::Mode MavlinkServer::translateCameraMode(uint32_t mode)
{
    CameraParameters::Mode ret = CameraParameters::Mode::MODE_STILL;

    switch (mode) {
    case CAMERA_MODE_IMAGE:
        ret = CameraParameters::Mode::MODE_STILL;
        break;
    case CAMERA_MODE_VIDEO:
        ret = CameraParameters::Mode::MODE_VIDEO;
        break;
    // case CAMERA_MODE_IMAGE_SURVEY:
    // ret = CameraParameters::Mode::MODE_IMG_SURVEY;
    // break;
    default:
        log_error("Unknown camera mode :%d", mode);
        break;
    }

    return ret;
}
