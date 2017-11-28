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
#include <string>

#include "CameraDevice.h"
#include "CameraParameters.h"

class CameraDevice_V4L2 final : public CameraDevice {
public:
    CameraDevice_V4L2(std::string device);
    ~CameraDevice_V4L2();
    int init(CameraParameters &camParam);
    int uninit();
    int start();
    int stop();
    std::vector<uint8_t> read();
    int getInfo(struct CameraInfo &camInfo);
    int setSize(uint32_t width, uint32_t height);
    int setPixelFormat(uint32_t format);
    int setMode(uint32_t mode);
    int getMode();
    int setBrightness(uint32_t value);
    int setContrast(uint32_t value);
    int setSaturation(uint32_t value);
    int setWhiteBalanceMode(uint32_t value);
    int setGamma(uint32_t value);
    int setGain(uint32_t value);
    int setPowerLineFrequency(uint32_t value);
    int setWhiteBalanceTemperature(uint32_t value);
    int setSharpness(uint32_t value);
    int setBacklightCompensation(uint32_t value);
    int setExposureMode(uint32_t value);
    int setExposureAbsolute(uint32_t value);
    int setSceneMode(uint32_t value);
    int setHue(int32_t value);

private:
    std::string mDevice;
    int mMode;
    int set_control(int ctrl_id, int value);
};
