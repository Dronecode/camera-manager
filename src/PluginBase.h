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
#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "CameraDevice.h"

/**
 *  The PluginBase is base class for plugins. Plugins are factories for a class of CameraDevices.
 *  For example - PluginV4l2 creates CameraDevice object for V4L2 cameras discovered in the system.
 *  Plugins that gets compiled will self-register on execution using static initialization.
 *  PluginManager maintains a table of Plugins and camera devices associated with each Plugin.
 *  To add support for custom type of camera, implement a custom Plugin derived from PluginBase.
 *  This custom Plugin will create custom CameraDevice.
 */

class PluginBase {
public:
    /**
     *  Destructor. Erases itself from the list.
     */
    virtual ~PluginBase()
    {
        std::vector<PluginBase *> pluginList = getPlugins();
        pluginList.erase(std::remove(pluginList.begin(), pluginList.end(), this), pluginList.end());
    }

    /**
     *  Get the list of plugin objects.
     *
     *  @return Reference to vector of plugin object.
     */
    static std::vector<PluginBase *> &getPlugins()
    {
        static std::vector<PluginBase *> pluginList;
        return pluginList;
    }

    /**
     *  Get the list of camera devices discovered by plugin.
     *
     *  @return Vector of names(ID) of camera devices.
     */
    virtual std::vector<std::string> getCameraDevices() = 0;

    /**
     *  Create a CameraDevice.
     *
     *  @param[in] deviceID Camera device identifier(name)
     *
     *  @return Pointer to CameraDevice object created or nullptr on failure
     */
    virtual std::shared_ptr<CameraDevice> createCameraDevice(std::string deviceID) = 0;

protected:
    /**
     *  Constructor. Self-registers by adding itself to the list.
     */
    PluginBase() { getPlugins().push_back(this); }
};
