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
#include "ImageCapture.h"

class ImageCaptureGst final : public ImageCapture {
public:
    ImageCaptureGst(std::shared_ptr<CameraDevice> camDev);
    ImageCaptureGst(std::shared_ptr<CameraDevice> camDev, struct ImageSettings &imgSetting);
    ~ImageCaptureGst();
    int start(int interval, int count, std::function<void(int result, int seq_num)> cb);
    int stop();
    int getState();
    int setResolution(int imgWidth, int imgHeight);
    int setFormat(CameraParameters::IMAGE_FILE_FORMAT imgFormat);
    int setLocation(const std::string imgPath);
    std::shared_ptr<CameraDevice> mCamDev;

private:
    int setState(int state);
    int click(int seq_num);
    void captureThread(int num, int interval);
    int createV4l2Pipeline(int seq_num);
    std::string getGstImgEncName(int format);
    std::string getGstPixFormat(int pixFormat);
    std::string getImgExt(int format);
    std::string getGstPipelineNameV4l2(int seq_num);
    int createAppsrcPipeline(int seq_num);
    std::string mDevice;
    std::atomic<int> mState;
    uint32_t mWidth;                             /* Image Width*/
    uint32_t mHeight;                            /* Image Height*/
    CameraParameters::IMAGE_FILE_FORMAT mFormat; /* Image File Format*/
    std::string mPath;                           /* Image File Destination Path*/
    uint32_t mCamWidth;                          /* Camera Frame Width*/
    uint32_t mCamHeight;                         /* Camera Frame Height*/
    uint32_t mCamPixFormat;                      /* Camera Frame Pixel Format*/
    std::function<void(int result, int seq_num)> mResultCB;
    std::thread mThread;
};
