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

#include "PluginManager.h"

// build a map of camera device(key) and their plugin(value)
PluginManager::PluginManager()
{
    // for all plugins, get the list of camera devices discovered
    for (PluginBase *plugin : PluginBase::getPlugins()) {
        for (std::string deviceID : plugin->getCameraDevices()) {
            // add the (device,factory) pair to map
            mPluginMap[deviceID] = plugin;
        }
    }
}

PluginManager::~PluginManager()
{
}

std::vector<std::string> PluginManager::listCameraDevices()
{
    std::vector<std::string> pluginList;
    for (auto const &element : mPluginMap) {
        pluginList.push_back(element.first);
    }
    return pluginList;
}

std::shared_ptr<CameraDevice> PluginManager::createCameraDevice(std::string deviceID)
{
    if (mPluginMap.find(deviceID) == mPluginMap.end()) {
        return nullptr;
    } else {
        return mPluginMap[deviceID]->createCameraDevice(deviceID);
    }
}
