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
#include <gazebo/msgs/msgs.hh>
#include <gazebo/transport/transport.hh>
#include <string>

#include "CameraDevice.h"
#include "CameraParameters.h"

class CameraDeviceGazebo final : public CameraDevice {
public:
    CameraDeviceGazebo(std::string device);
    ~CameraDeviceGazebo();
    std::string getDeviceId() const;
    Status getInfo(CameraInfo &camInfo) const;
    bool isGstV4l2Src() const;
    Status init(CameraParameters &camParam);
    Status uninit();
    Status start();
    Status stop();
    std::vector<uint8_t> read();
    Status setParam(CameraParameters &camParam, const std::string param, const char *param_value,
                    const size_t value_size, const int param_type);
    Status resetParams(CameraParameters &camParam);
    Status getSize(uint32_t &width, uint32_t &height) const;
    Status getPixelFormat(CameraParameters::PixelFormat &format) const;
    Status setMode(const CameraParameters::Mode mode);
    Status getMode(CameraParameters::Mode &mode) const;
    Status setCameraDefinitionUri(const std::string uri);
    std::string getCameraDefinitionUri() const;
    std::string getOverlayText() const;

private:
    // Declare parameter name & ID
    // Possible values of the params are in camera definition file
    static const char PARAMETER_CUSTOM_UINT8[];
    static const int ID_PARAMETER_CUSTOM_UINT8;
    static const char PARAMETER_CUSTOM_UINT32[];
    static const int ID_PARAMETER_CUSTOM_UINT32;
    static const char PARAMETER_CUSTOM_INT32[];
    static const int ID_PARAMETER_CUSTOM_INT32;
    static const char PARAMETER_CUSTOM_REAL32[];
    static const int ID_PARAMETER_CUSTOM_REAL32;
    static const char PARAMETER_CUSTOM_ENUM[];
    static const int ID_PARAMETER_CUSTOM_ENUM;
    int setOverlayText(std::string text);
    void cbOnImages(ConstImagesStampedPtr &_msg);
    int getImage(const gazebo::msgs::Image &_msg);
    std::string mDeviceId;
    std::atomic<CameraDevice::State> mState;
    uint32_t mWidth;
    uint32_t mHeight;
    CameraParameters::Mode mMode;
    CameraParameters::PixelFormat mPixelFormat;
    std::string mCamDefUri;
    std::string mTopicName;
    gazebo::transport::NodePtr mNode;
    gazebo::transport::SubscriberPtr mSub;
    std::mutex mLock;
    std::vector<uint8_t> mFrameBuffer = {};
    std::string mOvText;
};
