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
#include <linux/videodev2.h>
#include <string>

#include "CameraDevice.h"
#include "CameraParameters.h"

class CameraDeviceV4l2 final : public CameraDevice {
public:
    CameraDeviceV4l2(std::string device);
    ~CameraDeviceV4l2();
    int init(CameraParameters &camParam);
    int uninit();
    int start();
    int stop();
    std::vector<uint8_t> read();
    int setParam(CameraParameters &camParam, std::string param, const char *param_value,
                 size_t value_size, int param_type);
    std::string getDeviceId();
    int getInfo(struct CameraInfo &camInfo);
    bool isGstV4l2Src();
    int setSize(uint32_t width, uint32_t height);
    int setPixelFormat(uint32_t format);
    int setMode(uint32_t mode);
    int getMode();
    int resetParams(CameraParameters &camParam);

private:
    std::string mDeviceId;
    std::string mCardName;
    std::string mDriverName;
    uint32_t mVersion;
    int mMode;
    int initInfo();
    int initParams(CameraParameters &camParam);
    int declareParams(CameraParameters &camParam);
    int resetV4l2Params(CameraParameters &camParam);
    int declareV4l2Params(CameraParameters &camParam, struct v4l2_queryctrl &qctrl, int32_t value);
    std::string getParamName(int cid);
    CameraParameters::param_type getParamType(v4l2_ctrl_type type);
    int set_control(int ctrl_id, int value);
};
