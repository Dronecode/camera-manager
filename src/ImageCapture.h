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
#include <atomic>
#include <functional>
#include <string>
#include <thread>

#include "CameraDevice.h"

class ImageCapture {
public:
    ImageCapture() {}
    ~ImageCapture() {}

    enum State { STATE_ERROR = -1, STATE_IDLE = 0, STATE_IN_PROGRESS = 1 };

    virtual int start(int num, int interval, std::function<void(int result, int seq_num)> cb) = 0;
    virtual int stop() = 0;
    virtual int getState() = 0;
    virtual int setResolution(int imgWidth, int imgHeight) = 0;
    virtual int setFormat(int imgFormat) = 0;
    virtual int setDestination(const std::string imgPath) = 0;
};
