/*
 * This file is part of the Camera Streaming Daemon
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

#include <gst/gst.h>

#include "CameraDevice.h"
#include "VideoStream.h"

class VideoStreamUdp final : public VideoStream {
public:
    VideoStreamUdp(std::shared_ptr<CameraDevice> camDev);
    ~VideoStreamUdp();

    int init();
    int uninit();
    int start();
    int stop();
    int getState();
    int setResolution(int imgWidth, int imgHeight);
    int getResolution(int &imgWidth, int &imgHeight);
    int setFormat(int vidFormat);
    int getFormat();
    // The host/IP/Multicast group to send the packets to
    int setAddress(std::string ipAddr);
    std::string getAddress();
    // The port to send the packets to
    int setPort(uint32_t port);
    int getPort();
    int setTextOverlay(std::string text, int timeSec);
    std::string getTextOverlay();
    GstBuffer *readFrame();

private:
    int setState(int state);
    int createAppsrcPipeline();
    int destroyAppsrcPipeline();
    std::shared_ptr<CameraDevice> mCamDev;
    std::atomic<int> mState;
    uint32_t mWidth;
    uint32_t mHeight;
    std::string mHost;
    uint32_t mPort;
    std::string mOvText;
    int mOvTime;   // Time in sec to keep overlay, -1 forever
    int mOvFrmCnt; // framerate * mOvTime
    GstElement *mPipeline;
    GstElement *mTextOverlay;
};
