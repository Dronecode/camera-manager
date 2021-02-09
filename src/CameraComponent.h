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
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "CameraDevice.h"
#include "CameraParameters.h"
#include "ImageCapture.h"
#include "VideoCapture.h"
#include "VideoStream.h"
#include "log.h"

struct StorageInfo {
    uint8_t storage_id;
    uint8_t storage_count;
    uint8_t status;
    float total_capacity;
    float used_capacity;
    float available_capacity;
    float read_speed;
    float write_speed;
};

class CameraDevice;

class CameraComponent {
public:
    CameraComponent(std::shared_ptr<CameraDevice> device);
    virtual ~CameraComponent();
    int start();
    int stop();
    const CameraInfo &getCameraInfo() const;
    const StorageInfo &getStorageInfo() const;
    VideoStreamInfo &getVideoStreamInfo();
  
    const std::map<std::string, std::string> &getParamList() const;
    int getParamType(const char *param_id, size_t id_size);
    virtual int getParam(const char *param_id, size_t id_size, char *param_value,
                         size_t value_size);
    virtual int setParam(const char *param_id, size_t id_size, const char *param_value,
                         size_t value_size, int param_type);
    virtual int setCameraMode(CameraParameters::Mode mode);
    virtual CameraParameters::Mode getCameraMode();
    typedef std::function<void(int result, int seq_num)> capture_callback_t;
    int setImageCaptureLocation(std::string imgPath);
    int setImageCaptureSettings(ImageSettings &imgSetting);
    void getImageCaptureStatus(uint8_t &status, int &interval);
    virtual int startImageCapture(int interval, int count, capture_callback_t cb);
    virtual int stopImageCapture();
    void cbImageCaptured(int result, int seq_num);
    int setVideoCaptureLocation(std::string vidPath);
    int setVideoCaptureSettings(VideoSettings &vidSetting);
    virtual int startVideoCapture(int status_freq);
    virtual int stopVideoCapture();
    virtual uint8_t getVideoCaptureStatus();
    int startVideoStream(const bool isUdp);
    int stopVideoStream();
    uint8_t getVideoStreamStatus() const;
    int resetCameraSettings(void);

private:
    std::string mCamDevName;               /* Camera device name */
    CameraInfo mCamInfo;                   /* Camera Information Structure */
    StorageInfo mStoreInfo;                /* Storage Information Structure */
    VideoStreamInfo mVidStreamInfo;	   /* Videos stream info Structure */ 
    CameraParameters mCamParam;            /* Camera Parameters Object */
    std::shared_ptr<CameraDevice> mCamDev; /* Camera Device Object */
    std::shared_ptr<ImageCapture> mImgCap; /* Image Capture Object */
    std::function<void(int result, int seq_num)> mImgCapCB;
    std::string mImgPath;
    std::shared_ptr<ImageSettings> mImgSetting; /* Image Setting Structure */
    std::shared_ptr<VideoCapture> mVidCap; /* Video Capture Object */
    std::string mVidPath;
    std::shared_ptr<VideoSettings> mVidSetting; /* Video Setting Structure */
    std::shared_ptr<VideoStream> mVidStream; /* Video Streaming Object*/

    void initStorageInfo(struct StorageInfo &storeInfo);
    int setVideoFrameFormat(uint32_t param_value);
    int setVideoSize(uint32_t param_value);
    std::string toString(const char *buf, size_t buf_size);
};
