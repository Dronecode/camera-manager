/*
 * This file is part of the Dronecode Camera Manager
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
    std::string getDeviceId() const;
    Status getInfo(CameraInfo &camInfo) const;
    bool isGstV4l2Src() const;
    Status init(CameraParameters &camParam);
    Status uninit();
    Status start();
    Status stop();
    Status setParam(CameraParameters &camParam, const std::string param, const char *param_value,
                    const size_t value_size, const int param_type);
    Status resetParams(CameraParameters &camParam);
    Status setSize(const uint32_t width, const uint32_t height);
    Status setPixelFormat(const CameraParameters::PixelFormat format);
    Status setMode(const CameraParameters::Mode mode);
    Status getMode(CameraParameters::Mode &mode) const;
    Status setCameraDefinitionUri(const std::string uri);
    std::string getCameraDefinitionUri() const;

private:
    std::string mDeviceId;
    std::string mCardName;
    std::string mDriverName;
    std::string mCamDefURI;
    uint32_t mVersion;
    CameraParameters::Mode mMode;
    int initInfo();
    int initParams(CameraParameters &camParam);
    int declareParams(CameraParameters &camParam);
    int resetV4l2Params(CameraParameters &camParam);
    int declareV4l2Params(CameraParameters &camParam);
    std::string getParamName(int cid);
    int getParamId(int cid);
    CameraParameters::param_type getParamType(v4l2_ctrl_type type);
    int getV4l2ControlId(int paramId);
    int setV4l2Control(int ctrl_id, int value);
};
