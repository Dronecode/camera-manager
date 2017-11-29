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
#include <memory>
#include <string>
#include <vector>

#include "CameraDevice.h"
#include "CameraParameters.h"
#include "ImageCapture.h"
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

class CameraDevice;
class ImageCapture;
class CameraComponent {
public:
    CameraComponent(std::string);
    CameraComponent(std::string, std::string);
    virtual ~CameraComponent();
    const CameraInfo &getCameraInfo() const;
    const StorageInfo &getStorageInfo() const;
    const std::map<std::string, std::string> &getParamList() const;
    int getParamType(const char *param_id, size_t id_size);
    virtual int getParam(const char *param_id, size_t id_size, char *param_value,
                         size_t value_size);
    virtual int setParam(const char *param_id, size_t id_size, const char *param_value,
                         size_t value_size, int param_type);
    virtual int setCameraMode(uint32_t mode);
    virtual int getCameraMode();
    typedef std::function<void(int result, int seq_num)> capture_callback_t;
    virtual int startImageCapture(int interval, int count, capture_callback_t cb);
    virtual int stopImageCapture();
    void cbImageCaptured(int result, int seq_num);

private:
    std::string mCamDevName;               /* Camera device name */
    CameraInfo mCamInfo;                   /* Camera Information Structure */
    StorageInfo mStoreInfo;                /* Storage Information Structure */
    CameraParameters mCamParam;            /* Camera Parameters Object */
    std::string mCamDefURI;                /* Camera Definition URI */
    std::shared_ptr<CameraDevice> mCamDev; /* Camera Device Object */
    std::shared_ptr<ImageCapture> mImgCap; /* Image Capture Object */
    std::function<void(int result, int seq_num)> mImgCapCB;
    void initStorageInfo(struct StorageInfo &storeInfo);
    int setParam(std::string param_id, float param_value);
    int setParam(std::string param_id, int32_t param_value);
    int setParam(std::string param_id, uint32_t param_value);
    int setParam(std::string param_id, uint8_t param_value);
    int setVideoFrameFormat(uint32_t param_value);
    int setVideoSize(uint32_t param_value);
    int setImageFormat(uint32_t param_value);
    int setImazeSize(uint32_t param_value);
    std::shared_ptr<CameraDevice> create_camera_device(std::string camdev_name);
    std::string toString(const char *buf, size_t buf_size);
};
