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
#include "CameraComponent.h"
#include "CameraParameters.h"
#include <vector>

class CameraFrame {
public:
    CameraFrame() {}
    ~CameraFrame() {}
    uint32_t width;
    uint32_t height;
    uint32_t pixelFormat; // CameraParameters::ID_PIXEL_FORMAT_*
    uint8_t *buffer;
    size_t bufSize;
};

class CameraDevice {
public:
    CameraDevice() {}
    virtual ~CameraDevice() {}

    enum State {
        STATE_ERROR = -1,
        STATE_IDLE = 0,
        STATE_INIT = 1,
        STATE_RUN = 1,
    };

    virtual std::string getDeviceId() = 0;
    virtual int getInfo(struct CameraInfo &camInfo) = 0;
    virtual bool isGstV4l2Src() = 0;
    virtual int init(CameraParameters &camParam) = 0;
    virtual int uninit() = 0;
    virtual int start() { return -ENOTSUP; }
    virtual int start(std::function<void(std::vector<uint8_t>)> cb) { return -ENOTSUP; }
    virtual int stop() { return -ENOTSUP; }
    virtual std::vector<uint8_t> read() { return {}; }
    virtual int resetParams() { return -ENOTSUP; }
    virtual int setParam(const char *param_id, size_t id_size, const char *param_value,
                         size_t value_size, int param_type) { return -ENOTSUP; }
    virtual int setParam(std::string param_id, float param_value) { return -ENOTSUP; }
    virtual int setParam(std::string param_id, int32_t param_value) { return -ENOTSUP; }
    virtual int setParam(std::string param_id, uint32_t param_value) { return -ENOTSUP; }
    virtual int setParam(std::string param_id, uint8_t param_value) { return -ENOTSUP; }
    virtual int setSize(uint32_t width, uint32_t height) { return -ENOTSUP; }
    virtual int getSize(uint32_t &width, uint32_t &height) { return -ENOTSUP; }
    virtual int setPixelFormat(uint32_t format) { return -ENOTSUP; }
    virtual int getPixelFormat(uint32_t &format) { return -ENOTSUP; }
    virtual int setMode(uint32_t mode) { return -ENOTSUP; }
    virtual int getMode() { return -ENOTSUP; };
    virtual int setBrightness(uint32_t value) { return -ENOTSUP; }
    virtual int setContrast(uint32_t value) { return -ENOTSUP; }
    virtual int setSaturation(uint32_t value) { return -ENOTSUP; }
    virtual int setWhiteBalanceMode(uint32_t value) { return -ENOTSUP; }
    virtual int setGamma(uint32_t value) { return -ENOTSUP; }
    virtual int setGain(uint32_t value) { return -ENOTSUP; }
    virtual int setPowerLineFrequency(uint32_t value) { return -ENOTSUP; }
    virtual int setWhiteBalanceTemperature(uint32_t value) { return -ENOTSUP; }
    virtual int setSharpness(uint32_t value) { return -ENOTSUP; }
    virtual int setBacklightCompensation(uint32_t value) { return -ENOTSUP; }
    virtual int setExposureMode(uint32_t value) { return -ENOTSUP; }
    virtual int setExposureAbsolute(uint32_t value) { return -ENOTSUP; }
    virtual int setSceneMode(uint32_t value) { return -ENOTSUP; }
    virtual int setHue(int32_t value) { return -ENOTSUP; }
};
