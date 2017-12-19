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
#include <gazebo/msgs/msgs.hh>
#include <gazebo/transport/transport.hh>
#include <string>

#include "CameraDevice.h"
#include "CameraParameters.h"

class CameraDeviceGazebo final : public CameraDevice {
public:
    CameraDeviceGazebo(std::string device);
    ~CameraDeviceGazebo();
    std::string getDeviceId();
    int getInfo(struct CameraInfo &camInfo);
    int init(CameraParameters &camParam);
    int uninit();
    int start();
    int stop();
    std::vector<uint8_t> read();
    std::string getDevice();
    int getWidth();
    int getHeight();
    int getPixelFormat();

private:
    void cbOnImages(ConstImagesStampedPtr &_msg);
    int getImage(const gazebo::msgs::Image &_msg);
    std::string mDeviceId;
    int mWidth;
    int mHeight;
    int mPixelFormat;
    std::string mTopicName;
    gazebo::transport::NodePtr mNode;
    gazebo::transport::SubscriberPtr mSub;
    std::mutex mLock;
    std::vector<uint8_t> mFrameBuffer = {};
};
