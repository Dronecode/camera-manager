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

#include <atomic>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <memory>
#include <string>

#include "CameraDevice.h"
#include "VideoStream.h"
#include "log.h"

class VideoStreamRtsp final : public VideoStream {
public:
    VideoStreamRtsp(std::shared_ptr<CameraDevice> camDev);
    ~VideoStreamRtsp();

    int init();
    int uninit();
    int getInfo(VideoStreamInfo &vidStreamInfo);
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
    int getCameraResolution(uint32_t &width, uint32_t &height);
    CameraParameters::PixelFormat getCameraPixelFormat();
    std::string getGstPipeline(std::map<std::string, std::string> &params);
    GstBuffer *readFrame();
    std::shared_ptr<CameraDevice> getCameraDevice() { return mCamDev;  };

private:
    GstRTSPServer *createRtspServer();
    void destroyRtspServer();
    void attachRtspServer();
    int setState(int state);
    int startRtspServer();
    int stopRtspServer();
    std::shared_ptr<CameraDevice> mCamDev;
    std::atomic<int> mState;
    uint32_t mWidth;
    uint32_t mHeight;
    CameraParameters::VIDEO_CODING_FORMAT mEncFormat;
    std::string mHost;
    uint32_t mPort;
    std::string mPath;
    static GstRTSPServer *mServer;
    static bool isAttach;
    static uint32_t refCnt;
};
