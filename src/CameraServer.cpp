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

#include <set>

#include "CameraComponent.h"
#include "CameraServer.h"
#include "log.h"
#include "util.h"
#ifdef ENABLE_MAVLINK
#include "mavlink_server.h"
#endif
#include "v4l2_interface.h"

#define DEFAULT_SERVICE_PORT 8554

#ifdef ENABLE_MAVLINK
CameraServer::CameraServer(const ConfFile &conf)
    : mavlink_server(conf, streams, rtsp_server)
    , rtsp_server(streams, DEFAULT_SERVICE_PORT)
    , cameraCount(0)
{
    bool isImgCapSetting = false;
    bool isVidCapSetting = false;

    cameraCount = detectCamera(conf);

    // Read image capture settings
    ImageSettings imgSetting;
    if (!getImgCapSettings(conf, imgSetting)) {
        // Send the setting to camera comp
        isImgCapSetting = true;
    } else
        log_info("Image Capture Settings not found, use default");

    // Read image capture file location
    std::string imgPath = getImgCapLocation(conf);

    // Read video capture settings
    VideoSettings vidSetting;
    if (!getVidCapSettings(conf, vidSetting)) {
        // Send the setting to camera comp
        isVidCapSetting = true;
    } else
        log_info("Video Capture Settings not found, use default");

    std::string vidPath = getVidCapLocation(conf);

    std::vector<CameraComponent *>::iterator it;
    for (it = cameraList.begin(); it != cameraList.end(); it++) {
        if (*it) {
            if (mavlink_server.addCameraComponent(*it) == -1)
                log_error("Error in adding Camera Component");

            if (isImgCapSetting)
                (*it)->setImageCaptureSettings(imgSetting);

            if (!imgPath.empty())
                (*it)->setImageCaptureLocation(imgPath);

            if (isVidCapSetting)
                (*it)->setVideoCaptureSettings(vidSetting);

            if (!vidPath.empty())
                (*it)->setVideoCaptureLocation(vidPath);
        }
    }
}
#else
CameraServer::CameraServer(const ConfFile &conf)
    : rtsp_server(streams, DEFAULT_SERVICE_PORT)
    , cameraCount(0)
{
    cameraCount = detectCamera(conf);

    // Read image capture file location
    std::string imgPath = getImgCapLocation(conf);

    std::vector<CameraComponent *>::iterator it;
    for (it = cameraList.begin(); it != cameraList.end(); it++) {
        if (*it) {
         #ifdef ENABLE_MAVLINK
            if (mavlink_server.addCameraComponent(*it) == -1)
                log_error("Error in adding Camera Component");
        #endif

            if (!imgPath.empty())
                (*it)->setImageLocation(imgPath);
        }
    }
}
#endif

CameraServer::~CameraServer()
{
    stop();

    // Free up all the resources  allocated
}

void CameraServer::start()
{
    log_error("CAMERA SERVER START");
#ifdef ENABLE_MAVLINK
    mavlink_server.start();
#endif
}

void CameraServer::stop()
{
#ifdef ENABLE_MAVLINK
    mavlink_server.stop();
#endif
}

// prepare the list of cameras in the system
int CameraServer::detectCamera(const ConfFile &conf)
{
    int count = 0;

#ifdef ENABLE_GAZEBO
    // Check for SITL gazebo first, if not found search for other devices
    // It is possible to have gazebo camera with real camera but not enabling
    // this for now
    count += detect_devices_gazebo(conf, cameraList);
    if (count > 0)
        return count;
#endif

    count += detect_devices_v4l2(conf, cameraList);

    return count;
}

#ifdef ENABLE_GAZEBO
int CameraServer::detect_devices_gazebo(const ConfFile &conf,
                                        std::vector<CameraComponent *> &camList) const
{
    char *uri_addr = 0;
    std::string camTopic = getGazeboCamTopic(conf);
    if (camTopic.empty())
        return 0;

    log_debug("Found Gazebo camera :%s", camTopic.c_str());
    // TODO::Check if the topic found is valid
    if (!conf.extract_options("uri", "gazebo", &uri_addr)) {
        std::string uriString(uri_addr);
        camList.push_back(new CameraComponent(camTopic, uriString));
        free(uri_addr);
    } else {
        log_warning("Camera Definition for gazebo camera not found");
        camList.push_back(new CameraComponent(camTopic));
    }

    return 1;
}
#endif

int CameraServer::detect_devices_v4l2(const ConfFile &conf,
                                      std::vector<CameraComponent *> &camList) const
{
    int count = 0;
    char *uri_addr = 0;

    // Get the list of devices in the system
    std::vector<std::string> v4l2List;
    v4l2_list_devices(v4l2List);
    if (v4l2List.empty())
        return 0;

    // Get the blacklisted devices
    std::set<std::string> blacklist;
    static const ConfFile::OptionsTable option_table[] = {
        {"blacklist", false, ConfFile::parse_stl_set, 0, 0},
    };
    conf.extract_options("v4l2", option_table, 1, (void *)&blacklist);

    // Create camera components for devices not blacklisted
    std::string dev_name;
    for (auto dev_path : v4l2List) {
        log_debug("v4l2 device :: %s", dev_path.c_str());
        dev_name = dev_path.substr(sizeof(V4L2_DEVICE_PATH) - 1);
        if (blacklist.find(dev_name) != blacklist.end())
            continue;
        if (!conf.extract_options("uri", dev_name.c_str(), &uri_addr)) {
            std::string uriString(uri_addr);
            camList.push_back(new CameraComponent(dev_path, uriString));
            free(uri_addr);
        } else {
            log_warning("Camera Definition for device:%s not found", dev_name.c_str());
            camList.push_back(new CameraComponent(dev_path));
        }
        count++;
    }

    return count;
}

int CameraServer::getImgCapSettings(const ConfFile &conf, ImageSettings &imgSetting) const
{
    int ret = 0;

    struct options {
        int width;
        int height;
        int format;
    } opt = {};

    // All the settings must be available else default value for all will be used
    static const ConfFile::OptionsTable option_table[] = {
        {"width", true, ConfFile::parse_i, OPTIONS_TABLE_STRUCT_FIELD(options, width)},
        {"height", true, ConfFile::parse_i, OPTIONS_TABLE_STRUCT_FIELD(options, height)},
        {"format", true, ConfFile::parse_i, OPTIONS_TABLE_STRUCT_FIELD(options, format)},
    };
    ret = conf.extract_options("imgcap", option_table, ARRAY_SIZE(option_table), (void *)&opt);
    if (ret)
        return ret;

    imgSetting.width = opt.width;
    imgSetting.height = opt.height;
    imgSetting.fileFormat = static_cast<CameraParameters::IMAGE_FILE_FORMAT>(opt.format);
    log_info("Image Capture Width=%d Height=%d format=%d", imgSetting.width, imgSetting.height,
             imgSetting.fileFormat);

    return ret;
}

std::string CameraServer::getImgCapLocation(const ConfFile &conf) const
{
    // Location must start and end with "/"
    char *imgPath = 0;
    const char *key = "location";
    std::string ret;
    if (!conf.extract_options("imgcap", key, &imgPath)) {
        ret = std::string(imgPath);
        free(imgPath);
    } else {
        log_error("Image Capture location not found");
        ret = {};
    }

    return ret;
}

int CameraServer::getVidCapSettings(const ConfFile &conf, VideoSettings &vidSetting) const
{
    int ret = 0;

    struct options {
        int width;
        int height;
        int framerate;
        int bitrate;
        int encoder;
        int format;
    } opt = {};

    // All the settings must be available else default value for all will be used
    static const ConfFile::OptionsTable option_table[] = {
        {"width", true, ConfFile::parse_i, OPTIONS_TABLE_STRUCT_FIELD(options, width)},
        {"height", true, ConfFile::parse_i, OPTIONS_TABLE_STRUCT_FIELD(options, height)},
        {"framerate", true, ConfFile::parse_i, OPTIONS_TABLE_STRUCT_FIELD(options, framerate)},
        {"bitrate", true, ConfFile::parse_i, OPTIONS_TABLE_STRUCT_FIELD(options, bitrate)},
        {"encoder", true, ConfFile::parse_i, OPTIONS_TABLE_STRUCT_FIELD(options, encoder)},
        {"format", true, ConfFile::parse_i, OPTIONS_TABLE_STRUCT_FIELD(options, format)},
    };
    ret = conf.extract_options("vidcap", option_table, ARRAY_SIZE(option_table), (void *)&opt);
    if (ret)
        return ret;

    vidSetting.width = opt.width;
    vidSetting.height = opt.height;
    vidSetting.frameRate = opt.framerate;
    vidSetting.bitRate = opt.bitrate;
    vidSetting.encoder = static_cast<CameraParameters::VIDEO_CODING_FORMAT>(opt.encoder);
    vidSetting.fileFormat = static_cast<CameraParameters::VIDEO_FILE_FORMAT>(opt.format);
    log_info("Video Capture Width=%d Height=%d framerate=%d, bitrate=%dkbps, encoder=%d, format=%d",
             vidSetting.width, vidSetting.height, vidSetting.frameRate, vidSetting.bitRate,
             vidSetting.encoder, vidSetting.fileFormat);

    return ret;
}

std::string CameraServer::getVidCapLocation(const ConfFile &conf) const
{
    // Location must start and end with "/"
    char *vidPath = 0;
    const char *key = "location";
    std::string ret;
    if (!conf.extract_options("vidcap", key, &vidPath)) {
        ret = std::string(vidPath);
        free(vidPath);
    } else {
        log_warning("Video Capture location not found, use default");
        ret = {};
    }

    return ret;
}

std::string CameraServer::getGazeboCamTopic(const ConfFile &conf) const
{
    // Location must start and end with "/"
    char *topic = 0;
    const char *key = "camtopic";
    std::string ret;
    if (!conf.extract_options("gazebo", key, &topic)) {
        ret = std::string(topic);
        free(topic);
    } else {
        log_error("Gazebo Camera Topic not found");
        ret = {};
    }

    return ret;
}
