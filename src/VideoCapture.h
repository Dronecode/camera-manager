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

#include "CameraParameters.h"

struct VideoSettings {
    int width;
    int height;
    int frameRate;
    int bitRate; // in kbps
    CameraParameters::VIDEO_CODING_FORMAT encoder;
    CameraParameters::VIDEO_FILE_FORMAT fileFormat;
};

class VideoCapture {
public:
    VideoCapture() {}
    ~VideoCapture() {}

    enum State {
        STATE_ERROR = -1,
        STATE_IDLE = 0,
        STATE_INIT = 1,
        STATE_RUN = 2,
    };

    virtual int init() = 0;
    virtual int uninit() = 0;
    virtual int start() = 0;
    virtual int stop() = 0;
    virtual int getState() = 0;
    virtual int setResolution(int vidWidth, int vidHeight) = 0;
    virtual int getResolution(int &vidWidth, int &vidHeight) = 0;
    virtual int setEncoder(CameraParameters::VIDEO_CODING_FORMAT vidEnc) = 0;
    virtual int setFormat(CameraParameters::VIDEO_FILE_FORMAT fileFormat) = 0;
    virtual int setBitRate(int bitRate) = 0;
    virtual int setFrameRate(int frameRate) = 0;
    virtual int setLocation(const std::string vidPath) = 0;
    virtual std::string getLocation() = 0;
};
