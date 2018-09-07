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
#pragma once
#include <set>
#include <string>
#include <vector>

#include "conf_file.h"
#ifdef ENABLE_MAVLINK
#include "mavlink_server.h"
#endif
#ifdef ENABLE_AVAHI
#include "avahi_publisher.h"
#endif

#include "CameraComponent.h"
#include "PluginManager.h"

class CameraServer {
public:
    CameraServer(const ConfFile &conf);
    ~CameraServer();
    void start();
    void stop();

private:
    void addCameraInformation(const std::shared_ptr<CameraDevice> &device);
    std::set<std::string> readBlacklistDevices(const ConfFile &conf) const;
    std::string readURI(const ConfFile &conf, std::string deviceID);
    bool readImgCapSettings(const ConfFile &conf, ImageSettings &imgSetting) const;
    std::string readImgCapLocation(const ConfFile &conf) const;
    bool readVidCapSettings(const ConfFile &conf, VideoSettings &vidSetting) const;
    std::string readVidCapLocation(const ConfFile &conf) const;
    std::string readGazeboCamTopic(const ConfFile &conf) const;
    PluginManager mPluginManager;

#ifdef ENABLE_MAVLINK
    MavlinkServer mMavlinkServer;
#endif
#ifdef ENABLE_AVAHI
    std::unique_ptr<AvahiPublisher> mAvahiPublisher;
#endif

    std::map<std::string, std::vector<std::string>> mCamInfoMap;
    std::vector<CameraComponent *> compList;
};
