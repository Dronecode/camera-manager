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
#include <functional>
#include <gst/gst.h>
#include <string>
#include <thread>

#include "CameraDevice.h"
#include "VideoCapture.h"

class VideoCaptureGst final : public VideoCapture {
public:
    VideoCaptureGst(std::shared_ptr<CameraDevice> camDev);
    VideoCaptureGst(std::shared_ptr<CameraDevice> camDev, struct VideoSettings &vidSetting);
    ~VideoCaptureGst();

    int init();
    int uninit();
    int start();
    int stop();
    int getState();
    int setResolution(int imgWidth, int imgHeight);
    int getResolution(int &imgWidth, int &imgHeight);
    int setBitRate(int bitRate);
    int setFrameRate(int frameRate);
    int setEncoder(CameraParameters::VIDEO_CODING_FORMAT vidEnc);
    int setFormat(CameraParameters::VIDEO_FILE_FORMAT fileFormat);
    int setLocation(const std::string vidPath);
    std::string getLocation();

private:
    static int vidCount;
    int setState(int state);
    std::string getGstEncName(int format);
    std::string getGstParserName(int format);
    std::string getGstMuxerName(int format);
    std::string getFileExt(int format);
    std::string getGstV4l2PipelineName();
    int createV4l2Pipeline();
    int destroyPipeline();
    std::shared_ptr<CameraDevice> mCamDev;
    std::atomic<int> mState;
    int mWidth;
    int mHeight;
    int mBitRate;
    int mFrmRate;
    CameraParameters::VIDEO_CODING_FORMAT mEnc;
    CameraParameters::VIDEO_FILE_FORMAT mFileFmt;
    std::string mFilePath;
    GstElement *mPipeline;
};
