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

#include "PluginV4l2.h"
#include "v4l2_interface.h"

static PluginV4l2 v4l2;

PluginV4l2::PluginV4l2()
    : PluginBase()
{
    v4l2_list_devices(mCamList);
}

PluginV4l2::~PluginV4l2()
{
}

std::vector<std::string> PluginV4l2::getCameraDevices()
{
    return mCamList;
}

std::shared_ptr<CameraDevice> PluginV4l2::createCameraDevice(std::string deviceID)
{
    // check if the device exists in the list
    if (std::find(mCamList.begin(), mCamList.end(), deviceID) == mCamList.end()) {
        log_error("Camera Device not found : %s", deviceID.c_str());
        return nullptr;
    }

    return std::make_shared<CameraDeviceV4l2>(deviceID);
}
