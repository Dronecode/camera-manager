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

#include <cstddef>
#include <set>

#include "CameraServer.h"
#include "log.h"
#include "util.h"

#define DEFAULT_SERVICE_PORT 8554
#define DEFAULT_SERVICE_TYPE "_rtsp._udp"

#ifdef ENABLE_GAZEBO
#define GAZEBO_STRING "gazebo"
#endif

CameraServer::CameraServer(const ConfFile &conf)
    : mPluginManager()
#ifdef ENABLE_MAVLINK
    , mMavlinkServer(conf)
#endif
{
    std::string confDeviceId;
    // Read image capture settings/destination
    ImageSettings imgSetting;
    bool isImgCapSetting = readImgCapSettings(conf, imgSetting);
    std::string imgPath = readImgCapLocation(conf);

    // Read video capture settings/destination
    VideoSettings vidSetting;
    bool isVidCapSetting = readVidCapSettings(conf, vidSetting);
    std::string vidPath = readVidCapLocation(conf);

    // Read blacklisted camera devices
    std::set<std::string> blackList = readBlacklistDevices(conf);

    std::vector<std::string> deviceList = mPluginManager.listCameraDevices();
    for (auto deviceID : deviceList) {
        log_debug("Camera Device : %s", deviceID.c_str());

        // TODO :: check if max camera count reached

        // check if blacklisted
        if (blackList.find(deviceID) != blackList.end()) {
            log_info("Device is black listed : %s", deviceID.c_str());
            continue;
        }

        // create camera device
        std::shared_ptr<CameraDevice> device = mPluginManager.createCameraDevice(deviceID);
        if (!device) {
            log_error("Error in creating device : %s", deviceID.c_str());
            continue;
        }

        // save camera device details in a table
        addCameraInformation(device);

        confDeviceId = deviceID;

#ifdef ENABLE_GAZEBO
        // The ID of gazebo device used in conf file is "gazebo" instead
        // of the topic name being used as deviceID in CameraDevice
        if (deviceID.find(GAZEBO_STRING) != std::string::npos) {
            confDeviceId = GAZEBO_STRING;
        }
#endif

        // Set the URI read from conf file
        device->setCameraDefinitionUri(readURI(conf, confDeviceId));

        // create camera component with camera device
        CameraComponent *comp = new CameraComponent(device);

        // configure camera component with settings
        if (isImgCapSetting)
            comp->setImageCaptureSettings(imgSetting);

        if (!imgPath.empty())
            comp->setImageCaptureLocation(imgPath);

        if (isVidCapSetting)
            comp->setVideoCaptureSettings(vidSetting);

        if (!vidPath.empty())
            comp->setVideoCaptureLocation(vidPath);

// add to mavlink server
#ifdef ENABLE_MAVLINK
        if (mMavlinkServer.addCameraComponent(comp) == -1) {
            log_error("Error in adding Camera Component");
            // TODO :: delete component and break
        }
#endif

        // Add component to the list
        compList.push_back(comp);

    }
}

CameraServer::~CameraServer()
{
    stop();

    // Free up all the resources  allocated
    for (auto camComp : compList) {
        delete camComp;
    }
}

void CameraServer::start()
{
    log_info("CAMERA SERVER START");
    for (auto camComp : compList) {
        if (camComp->start())
            log_error("Error in starting camera component");

        camComp->startVideoStream(false);
    }

#ifdef ENABLE_MAVLINK
    mMavlinkServer.start();
#endif

#ifdef ENABLE_AVAHI
    /* create avahi publisher */
    mAvahiPublisher = std::unique_ptr<AvahiPublisher>(
        new AvahiPublisher(mCamInfoMap, DEFAULT_SERVICE_PORT, DEFAULT_SERVICE_TYPE));

    /* start avahi publisher */
    mAvahiPublisher->start();

#endif
}

void CameraServer::stop()
{

#ifdef ENABLE_AVAHI
    /* stop avahi publisher */
    mAvahiPublisher->stop();

    /* delete avahi publisher */
    mAvahiPublisher.reset();
#endif

#ifdef ENABLE_MAVLINK
    mMavlinkServer.stop();
#endif

    for (auto camComp : compList) {
        camComp->stopVideoStream();
        camComp->stop();
    }

}

void CameraServer::addCameraInformation(const std::shared_ptr<CameraDevice> &device)
{
    std::string name = "/" + device->getDeviceId();

    std::vector<std::string> txtList;

    CameraInfo camInfo;
    device->getInfo(camInfo);

    std::string vendor
        = "vendor=" + std::string(reinterpret_cast<const char *>(camInfo.vendorName), 32);
    txtList.push_back(vendor);
    std::string model
        = "model=" + std::string(reinterpret_cast<const char *>(camInfo.modelName), 32);
    txtList.push_back(model);
    /* TODO:: Fill more details like supported sizes, formats etc*/

    mCamInfoMap[name] = txtList;

    return;
}

std::set<std::string> CameraServer::readBlacklistDevices(const ConfFile &conf) const
{
    std::set<std::string> blacklist;
    static const ConfFile::OptionsTable option_table[] = {
        {"blacklist", false, ConfFile::parse_stl_set, 0, 0},
    };
    conf.extract_options("v4l2", option_table, 1, (void *)&blacklist);
    return blacklist;
}

std::string CameraServer::readURI(const ConfFile &conf, std::string deviceID)
{
    char *uriAddr = 0;
    if (!conf.extract_options("uri", deviceID.c_str(), &uriAddr)) {
        std::string uriString(uriAddr);
        free(uriAddr);
        return uriString;
    } else
        return {};
}

bool CameraServer::readImgCapSettings(const ConfFile &conf, ImageSettings &imgSetting) const
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
        return false;

    imgSetting.width = opt.width;
    imgSetting.height = opt.height;
    imgSetting.fileFormat = static_cast<CameraParameters::IMAGE_FILE_FORMAT>(opt.format);
    log_info("Image Capture Width=%d Height=%d format=%d", imgSetting.width, imgSetting.height,
             imgSetting.fileFormat);

    return true;
}

std::string CameraServer::readImgCapLocation(const ConfFile &conf) const
{
    // Location must start and end with "/"
    char *imgPath = 0;
    const char *key = "location";
    std::string ret;
    if (!conf.extract_options("imgcap", key, &imgPath)) {
        ret = std::string(imgPath);
        log_info("Image Capture location : %s", imgPath);
        free(imgPath);
    } else {
        log_warning("Image Capture location not found, use default");
        ret = {};
    }

    return ret;
}

bool CameraServer::readVidCapSettings(const ConfFile &conf, VideoSettings &vidSetting) const
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
        return false;

    vidSetting.width = opt.width;
    vidSetting.height = opt.height;
    vidSetting.frameRate = opt.framerate;
    vidSetting.bitRate = opt.bitrate;
    vidSetting.encoder = static_cast<CameraParameters::VIDEO_CODING_FORMAT>(opt.encoder);
    vidSetting.fileFormat = static_cast<CameraParameters::VIDEO_FILE_FORMAT>(opt.format);
    log_info("Video Capture Width=%d Height=%d framerate=%d, bitrate=%dkbps, encoder=%d, format=%d",
             vidSetting.width, vidSetting.height, vidSetting.frameRate, vidSetting.bitRate,
             vidSetting.encoder, vidSetting.fileFormat);

    return true;
}

std::string CameraServer::readVidCapLocation(const ConfFile &conf) const
{
    // Location must start and end with "/"
    char *vidPath = 0;
    const char *key = "location";
    std::string ret;
    if (!conf.extract_options("vidcap", key, &vidPath)) {
        ret = std::string(vidPath);
        log_info("Video Capture location : %s", vidPath);
        free(vidPath);
    } else {
        log_warning("Video Capture location not found, use default");
        ret = {};
    }

    return ret;
}

std::string CameraServer::readGazeboCamTopic(const ConfFile &conf) const
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
