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
#include <gazebo/common/Image.hh>
#include <gazebo/common/common.hh>
#include <gazebo/gazebo_client.hh>
#include <gazebo/msgs/msgs.hh>

#include <iostream>

#include "CameraDeviceGazebo.h"

CameraDeviceGazebo::CameraDeviceGazebo(std::string device)
    : mDeviceId(device)
    , mWidth(640)
    , mHeight(360)
    , mPixelFormat(CameraParameters::ID_PIXEL_FORMAT_RGB24)
{
    log_debug("%s path:%s", __func__, mDeviceId.c_str());
}

CameraDeviceGazebo::~CameraDeviceGazebo()
{
    stop();
    uninit();
}

std::string CameraDeviceGazebo::getDeviceId()
{
    return mDeviceId;
}

int CameraDeviceGazebo::getInfo(struct CameraInfo &camInfo)
{
    strcpy((char *)camInfo.vendorName, "Gazebo");
    strcpy((char *)camInfo.modelName, "SITL-Camera");
    camInfo.firmware_version = 1;
    camInfo.focal_length = 0;
    camInfo.sensor_size_h = 0;
    camInfo.sensor_size_v = 0;
    camInfo.resolution_h = 0;
    camInfo.resolution_v = 0;
    camInfo.lens_id = 0;
    camInfo.flags = ~0u;
    camInfo.cam_definition_version = 1;
    return 0;
}

bool CameraDeviceGazebo::isGstV4l2Src()
{
    return false;
}

int CameraDeviceGazebo::init(CameraParameters &camParam)
{
    return 0;
}

int CameraDeviceGazebo::uninit()
{
    return 0;
}

int CameraDeviceGazebo::start()
{
    // Load Gazebo
    gazebo::client::setup();

    // Create our node for communication
    mNode.reset(new gazebo::transport::Node());
    mNode->Init();

    // TODO:: Get the number of buffers that gazebo camera has
    // TODO:: Initialize size of mFrameBuffer based on that

    // Listen to Gazebo <device> topic
    mSub = mNode->Subscribe(mDeviceId, &CameraDeviceGazebo::cbOnImages, this);
    return 0;
}

int CameraDeviceGazebo::stop()
{
    // Make sure to shut everything down.
    gazebo::client::shutdown();
    return 0;
}

std::vector<uint8_t> CameraDeviceGazebo::read()
{
    std::lock_guard<std::mutex> locker(mLock);
    return mFrameBuffer;
}

int CameraDeviceGazebo::getSize(uint32_t &width, uint32_t &height)
{
    width = mWidth;
    height = mHeight;

    return 0;
}

int CameraDeviceGazebo::getPixelFormat(uint32_t &format)
{
    format = mPixelFormat;

    return 0;
}

void CameraDeviceGazebo::cbOnImages(ConstImagesStampedPtr &_msg)
{
    std::lock_guard<std::mutex> locker(mLock);

    log_debug("Image Count: %d", _msg->image_size());
    for (int i = 0; i < _msg->image_size(); ++i) {
        getImage(_msg->image(i));
    }
}

int CameraDeviceGazebo::getImage(const gazebo::msgs::Image &_msg)
{
    int success = 0;
    int failure = 1;

    if (_msg.width() == 0 || _msg.height() == 0)
        return failure;

    // log_debug("Width: %u", _msg.width());
    // log_debug("Height: %u", _msg.height());
    mWidth = _msg.width();
    mHeight = _msg.height();

    switch (_msg.pixel_format()) {
    case gazebo::common::Image::L_INT8:
    case gazebo::common::Image::L_INT16: {
        log_error("Pixel Format Mono :: Unsupported");
        return failure;
    }
    case gazebo::common::Image::RGB_INT8:
    case gazebo::common::Image::RGBA_INT8: {
        mPixelFormat = CameraParameters::ID_PIXEL_FORMAT_RGB24;
        break;
    }
    default: {
        log_error("Pixel Format %d :: Unsupported", _msg.pixel_format());
        return failure;
    }
    }

    // copy image data to frame buffer
    // log_debug("Image Size: %lu Format:%d", _msg.data().size(), pixFormat);
    const char *buffer = (const char *)_msg.data().c_str();
    uint buffer_size = _msg.data().size();
    mFrameBuffer = std::vector<uint8_t>(buffer, buffer + buffer_size);

#if 0
    std::ofstream fout("imgframe.rgb", std::ios::binary);
    fout.write(reinterpret_cast<char*>(&mFrameBuffer[0]), mFrameBuffer.size() * sizeof(mFrameBuffer[0]));
    fout.close();
    std::cout<<"\nsaved";
#endif

    return success;
}
