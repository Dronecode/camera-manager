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
#include <iostream>
#include <string>
#include <vector>

#include "CameraDevice.h"

class PluginBase {
public:
    virtual ~PluginBase()
    {
        std::vector<PluginBase *> pluginList = getPlugins();
        pluginList.erase(std::remove(pluginList.begin(), pluginList.end(), this), pluginList.end());
    }

    static std::vector<PluginBase *> &getPlugins()
    {
        static std::vector<PluginBase *> pluginList;
        return pluginList;
    }
    virtual std::vector<std::string> getCameraDevices() = 0;
    virtual std::shared_ptr<CameraDevice> createCameraDevice(std::string deviceID) = 0;

protected:
    PluginBase() { getPlugins().push_back(this); }
};
