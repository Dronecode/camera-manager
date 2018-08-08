/*
 * This file is part of the Dronecode Camera Manager
 *
 * Copyright (C) 2018  Intel Corporation. All rights reserved.
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

#include <algorithm>
#include <array>
#include <cstdio>
#include <memory>
#include <string>

#include "CameraDeviceGazebo.h"
#include "PluginGazebo.h"

#define GZB_CMD_TOPIC "gz topic -l | grep \"camera/image\" "

static PluginGazebo gzb;

PluginGazebo::PluginGazebo()
    : PluginBase()
{
    discoverCameras(mCamList);
}

PluginGazebo::~PluginGazebo()
{
}

std::vector<std::string> PluginGazebo::getCameraDevices()
{
    return mCamList;
}

std::shared_ptr<CameraDevice> PluginGazebo::createCameraDevice(std::string deviceID)
{
    // check if the device exists in the list
    if (std::find(mCamList.begin(), mCamList.end(), deviceID) == mCamList.end()) {
        log_error("Camera Device not found : %s", deviceID.c_str());
        return nullptr;
    }

    return std::make_shared<CameraDeviceGazebo>(deviceID);
}

void PluginGazebo::discoverCameras(std::vector<std::string> &camList)
{
    std::string result;
    std::array<char, 128> buffer;
    std::shared_ptr<FILE> pipe(popen(GZB_CMD_TOPIC, "r"), pclose);
    if (!pipe) {
        log_error("popen() failed!");
        return;
    }
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr) {
            result = buffer.data();
            // Remove newline at the end
            result.pop_back();
            camList.push_back(result);
        }
    }

    return;
}
