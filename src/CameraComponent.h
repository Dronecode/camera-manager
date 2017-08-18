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
#include <gst/gst.h>
#include <map>
#include <string>
#include <vector>

#include "CameraParameters.h"
#include "log.h"

struct CameraInfo {
    uint8_t vendorName[32];
    uint8_t modelName[32];
    uint32_t firmware_version;
    float focal_length;
    float sensor_size_h;
    float sensor_size_v;
    uint16_t resolution_h;
    uint16_t resolution_v;
    uint8_t lens_id;
    uint32_t flags;
    uint16_t cam_definition_version;
    char cam_definition_uri[140];
};

struct StorageInfo {
    uint8_t storage_id;
    uint8_t storage_count;
    uint8_t status;
    float total_capacity;
    float used_capacity;
    float available_capacity;
    float read_speed;
    float write_speed;
};

class CameraComponent {
public:
    CameraComponent() {}
    virtual ~CameraComponent() {}
    const CameraInfo &getCameraInfo() const { return camInfo; }
    const StorageInfo &getStorageInfo() const { return storeInfo; }
    const std::map<std::string, std::string> &getParamList() { return camParam.getParameterList(); }
    virtual int getParam(const char *param_id, char *param_value, size_t value_size) = 0;
    virtual int setParam(const char *param_id, const char *param_value, size_t value_size,
                         int param_type)
        = 0;
    int getParamType(const char *param_id) { return camParam.getParameterType(param_id); }

protected:
    CameraInfo camInfo;
    StorageInfo storeInfo;
    CameraParameters camParam;
};
