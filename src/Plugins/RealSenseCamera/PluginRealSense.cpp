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

#include "CameraDeviceRealSense.h"
#include "PluginRealSense.h"

PluginRealSense::PluginRealSense()
    : PluginBase()
{
    /*
     * 1. Discover the list of camera devices for this Plugin Class
     * 2. Add the IDs of the devices in the list
     */

    discoverCameras(mCamList);
}

PluginRealSense::~PluginRealSense()
{
}

std::vector<std::string> PluginRealSense::getCameraDevices()
{
    return mCamList;
}

std::shared_ptr<CameraDevice> PluginRealSense::createCameraDevice(std::string deviceID)
{
    /* check if the device exists in the list */
    if (std::find(mCamList.begin(), mCamList.end(), deviceID) == mCamList.end()) {
        log_error("Camera Device not found : %s", deviceID.c_str());
        return nullptr;
    }

    return std::make_shared<CameraDeviceRealSense>(deviceID);
}

void PluginRealSense::discoverCameras(std::vector<std::string> &camList)
{
    /*
     * Add the logic to discover the camera devices
     * For RealSense cameras, its hardcoded
     */

    /* Depth Camera */
    camList.push_back("rsdepth");
    /* Infrared Camera*/
    camList.push_back("rsir");
    /* Infrared Camera2*/
    camList.push_back("rsir2");

    return;
}
