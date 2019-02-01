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

#include <string>
#include <vector>

#include "CameraDeviceCustom.h"
#include "PluginCustom.h"

static PluginCustom custom;

PluginCustom::PluginCustom()
    : PluginBase()
{
    /*
     * 1. Discover the list of camera devices for this Plugin Class
     * 2. Add the IDs of the devices in the list
     */

    discoverCameras(mCamList);
}

PluginCustom::~PluginCustom()
{
}

std::vector<std::string> PluginCustom::getCameraDevices()
{
    return mCamList;
}

std::shared_ptr<CameraDevice> PluginCustom::createCameraDevice(std::string deviceID)
{
    // check if the device exists in the list
    if (std::find(mCamList.begin(), mCamList.end(), deviceID) == mCamList.end()) {
        log_error("Camera Device not found : %s", deviceID.c_str());
        return nullptr;
    }

    return std::make_shared<CameraDeviceCustom>(deviceID);
}

void PluginCustom::discoverCameras(std::vector<std::string> &camList)
{
    /*
     * 1. Add the logic to discover the camera devices
     * 2. For V4L2, its scanning the video* nodes in /dev/ dir
     * 3. For Gazebo, the topics with "camera/image" is assumed to be a camera
     */

    return;
}
