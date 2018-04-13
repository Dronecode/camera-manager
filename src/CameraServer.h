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
#include <string>
#include <vector>

#include "conf_file.h"
#ifdef ENABLE_MAVLINK
#include "mavlink_server.h"
#endif
#include "rtsp_server.h"
#include "stream.h"

#include "CameraComponent.h"
#include "CameraParameters.h"
#include "VideoCapture.h"
#include "log.h"

class CameraServer {
public:
    CameraServer(const ConfFile &conf);
    ~CameraServer();
    void start();
    void stop();
    int getCameraCount() const { return cameraCount; }

private:
    int getImgCapSettings(const ConfFile &conf, ImageSettings &imgSetting) const;
    std::string getImgCapLocation(const ConfFile &conf) const;
    int getVidCapSettings(const ConfFile &conf, VideoSettings &vidSetting) const;
    std::string getVidCapLocation(const ConfFile &conf) const;
    std::string getGazeboCamTopic(const ConfFile &conf) const;
    int detectCamera(const ConfFile &conf);
#ifdef ENABLE_GAZEBO
    int detect_devices_gazebo(const ConfFile &conf, std::vector<CameraComponent *> &camList) const;
#endif
    int detect_devices_v4l2(const ConfFile &conf, std::vector<CameraComponent *> &cameraList) const;
#ifdef ENABLE_MAVLINK
    MavlinkServer mavlink_server;
#endif
    RTSPServer rtsp_server;
    int cameraCount;
    std::vector<CameraComponent *> cameraList;
    std::vector<std::unique_ptr<Stream>> streams; // Remove it
};
