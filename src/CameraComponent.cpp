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
#include <cstring>

#include "CameraComponent.h"
#include "ImageCaptureGst.h"
#include "VideoCaptureGst.h"
#include "VideoStreamRtsp.h"
#include "VideoStreamUdp.h"
#ifdef ENABLE_MAVLINK
#include "mavlink_server.h"
#endif
#include "util.h"
#include <algorithm>

CameraComponent::CameraComponent(std::shared_ptr<CameraDevice> device)
    : mCamDev(device)
{
    mCamDevName = mCamDev->getDeviceId();

    // Get info from the camera device
    mCamDev->getInfo(mCamInfo);

    initStorageInfo(mStoreInfo);
}

CameraComponent::~CameraComponent()
{
    log_debug("%s::%s", __func__, mCamDev->getDeviceId().c_str());

    if (mVidCap) {
        mVidCap->stop();
        mVidCap->uninit();
        mVidCap.reset();
    }

    if (mImgCap) {
        mImgCap->stop();
        mImgCap->uninit();
        mImgCap.reset();
    }

    if (mVidStream) {
        mVidStream->stop();
        mVidStream->uninit();
        mVidStream.reset();
    }

    // stop the camera device
    mCamDev->stop();

    // Uninit the camera device
    mCamDev->uninit();

    mCamDev.reset();
}

int CameraComponent::start()
{
    CameraDevice::Status ret;

    // Get list of Parameters supported & its default value
    ret = mCamDev->init(mCamParam);
    if (ret != CameraDevice::Status::SUCCESS)
        return -1;

    // start the camera device
    ret = mCamDev->start();
    if (ret != CameraDevice::Status::SUCCESS)
        return -1;

    return 0;
}

int CameraComponent::stop()
{
    if (mVidCap) {
        mVidCap->stop();
        mVidCap->uninit();
        mVidCap.reset();
    }

    if (mImgCap) {
        mImgCap->stop();
        mImgCap->uninit();
        mImgCap.reset();
    }

    if (mVidStream) {
        mVidStream->stop();
        mVidStream->uninit();
        mVidStream.reset();
    }

    // stop the camera device
    mCamDev->stop();

    // Uninit the camera device
    mCamDev->uninit();

    return 0;
}

const CameraInfo &CameraComponent::getCameraInfo() const
{
    return mCamInfo;
}

const StorageInfo &CameraComponent::getStorageInfo() const
{
    return mStoreInfo;
}

VideoStreamInfo &CameraComponent::getVideoStreamInfo()
{
   log_info("[%s::%s]", typeid(this).name(), __func__);
    if(mVidStream) 
        mVidStream->getInfo(mVidStreamInfo);
    return mVidStreamInfo;
}

const std::map<std::string, std::string> &CameraComponent::getParamList() const
{
    return mCamParam.getParameterList();
}

void CameraComponent::initStorageInfo(struct StorageInfo &storeInfo)
{
    // TODO:: Fill storage details with real values
    storeInfo.storage_id = 1;
    storeInfo.storage_count = 1;
    storeInfo.status = 2; /*formatted*/
    storeInfo.total_capacity = 50.0;
    storeInfo.used_capacity = 0.0;
    storeInfo.available_capacity = 50.0;
    storeInfo.read_speed = 128;
    storeInfo.write_speed = 128;
}

int CameraComponent::getParamType(const char *param_id, size_t id_size)
{
    if (!param_id)
        return 0;

    return mCamParam.getParameterType(toString(param_id, id_size));
}

int CameraComponent::getParam(const char *param_id, size_t id_size, char *param_value,
                              size_t value_size)
{
    // query the value set in the map and fill the output, return appropriate value
    if (!param_id || !param_value || value_size == 0)
        return 1;

    std::string value = mCamParam.getParameter(toString(param_id, id_size));
    if (value.empty())
        return 1;

    mem_cpy(param_value, value_size, value.data(), value.size(), value_size);
    return 0;
}

int CameraComponent::setParam(const char *param_name, size_t id_size, const char *param_value,
                              size_t value_size, int param_type)
{
    CameraDevice::Status ret;
    std::string param = toString(param_name, id_size);
    ret = mCamDev->setParam(mCamParam, param, param_value, value_size, param_type);
    if (ret == CameraDevice::Status::SUCCESS)
        return 0;
    else
        return -1;
}

int CameraComponent::setCameraMode(CameraParameters::Mode mode)
{
    mCamDev->setMode(mode);
    return 0;
}

CameraParameters::Mode CameraComponent::getCameraMode()
{
    CameraParameters::Mode mode;
    mCamDev->getMode(mode);

    return mode;
}

int CameraComponent::resetCameraSettings()
{
    CameraDevice::Status ret = mCamDev->resetParams(mCamParam);
    if (ret != CameraDevice::Status::SUCCESS)
        log_debug("Error in reset of camera parameters. Could not open the device.");
    return -1;
}

int CameraComponent::setImageCaptureLocation(std::string imgPath)
{
    mImgPath = imgPath;
    return 0;
}

int CameraComponent::setImageCaptureSettings(ImageSettings &imgSetting)
{
    if (mImgSetting)
        mImgSetting.reset();

    mImgSetting = std::make_shared<ImageSettings>();
    *mImgSetting = imgSetting;

    return 0;
}

/* 0: idle, 1: capture in progress, 2: interval set but idle, 3: interval set and capture in
 * progress */
void CameraComponent::getImageCaptureStatus(uint8_t &status, int &interval)
{
    if (!mImgCap) {
        status = 0;
        interval = 0;
        return;
    }

    // get interval
    interval = mImgCap->getInterval();
    switch (mImgCap->getState()) {
    case ImageCapture::STATE_ERROR:
    case ImageCapture::STATE_IDLE:
    case ImageCapture::STATE_INIT:
        status = 0;
        break;
    case ImageCapture::STATE_RUN:
        if (interval > 0)
            status = 3; // or 2?
        else
            status = 1;
        break;
    default:
        status = 0;
        break;
    }

    log_debug("%s Status:%d Interval:%d", __func__, status, interval);
    return;
}

int CameraComponent::startImageCapture(int interval, int count, capture_callback_t cb)
{
    int ret = 0;

    // TODO :: Check if video capture or video streaming is running

    mImgCapCB = cb;

    // Delete imgCap instance if already exists
    // This could be because of no StopImageCapture call after done
    // Or new startImageCapture call while prev call is still not done
    if (mImgCap)
        mImgCap.reset();

    // check if settings are available
    if (mImgSetting)
        mImgCap = std::make_shared<ImageCaptureGst>(mCamDev, *mImgSetting);
    else
        mImgCap = std::make_shared<ImageCaptureGst>(mCamDev);

    if (!mImgPath.empty())
        mImgCap->setLocation(mImgPath);

    ret = mImgCap->init();
    if (!ret) {
        ret = mImgCap->start(interval, count,
                             std::bind(&CameraComponent::cbImageCaptured, this,
                                       std::placeholders::_1, std::placeholders::_2));
        if (ret) {
            mImgCap->uninit();
            mImgCap.reset();
        }
    }

    return ret;
}

int CameraComponent::stopImageCapture()
{
    if (!mImgCap)
        return 0;

    mImgCap->stop();
    mImgCap->uninit();
    mImgCap.reset();

    return 0;
}

void CameraComponent::cbImageCaptured(int result, int seq_num)
{
    log_debug("%s result:%d sequenc:%d", __func__, result, seq_num);
    // TODO :: Get the file path of the image and host it via http
    if (mImgCapCB)
        mImgCapCB(result, seq_num);
}

int CameraComponent::setVideoCaptureLocation(std::string vidPath)
{
    mVidPath = vidPath;
    return 0;
}

int CameraComponent::setVideoCaptureSettings(VideoSettings &vidSetting)
{
    if (mVidSetting)
        mVidSetting.reset();

    mVidSetting = std::make_shared<VideoSettings>();
    *mVidSetting = vidSetting;

    return 0;
}

int CameraComponent::startVideoCapture(int status_freq)
{
    int ret = 0;

    if (mVidCap)
        mVidCap.reset();

    // TODO :: Check if video capture or video streaming is running

    // check if settings are available
    if (mVidSetting)
        mVidCap = std::make_shared<VideoCaptureGst>(mCamDev, *mVidSetting);
    else
        mVidCap = std::make_shared<VideoCaptureGst>(mCamDev);

    if (!mVidPath.empty())
        mVidCap->setLocation(mVidPath);

    ret = mVidCap->init();
    if (!ret) {
        ret = mVidCap->start();
        if (ret) {
            mVidCap->uninit();
            mVidCap.reset();
        }
    }

    return ret;
}

int CameraComponent::stopVideoCapture()
{
    int ret = 0;

    if (!mVidCap)
        return 0;

    mVidCap->stop();
    mVidCap->uninit();
    mVidCap.reset();

    return ret;
}

/* 0: idle, 1: capture in progress */
uint8_t CameraComponent::getVideoCaptureStatus()
{
    uint8_t ret = 0;

    if (!mVidCap)
        return 0;

    switch (mVidCap->getState()) {
    case VideoCapture::STATE_ERROR:
    case VideoCapture::STATE_IDLE:
    case VideoCapture::STATE_INIT:
        ret = 0;
        break;
    case VideoCapture::STATE_RUN:
        ret = 1;
        break;
    default:
        ret = 0;
        break;
    }

    log_debug("%s Status:%d", __func__, ret);
    return ret;
}

int CameraComponent::setVideoSize(uint32_t param_value)
{
    return 0;
}

int CameraComponent::setVideoFrameFormat(uint32_t param_value)
{
    return 0;
}

int CameraComponent::startVideoStream(const bool isUdp)
{
    int ret = 0;

    // TODO :: Check if image/video capture is running
    // If yes, video streaming may work by adding a tee element
    // in the gstreamer pipeline. Currently conncurent use cases
    // not supported

    // Close previous instance of video streaming if exist
    if (mVidStream)
        mVidStream.reset();

    if (isUdp)
        mVidStream = std::make_shared<VideoStreamUdp>(mCamDev);
    else {
        mVidStream = std::make_shared<VideoStreamRtsp>(mCamDev);
    }

    ret = mVidStream->init();
    if (!ret) {
        ret = mVidStream->start();
        if (ret) {
            mVidStream->uninit();
            mVidStream.reset();
        }
    }

    return ret;
}

int CameraComponent::stopVideoStream()
{
    int ret = 0;

    if (!mVidStream)
        return 0;

    ret = mVidStream->stop();
    if (ret) {
        log_error("Error in Video Stream stop");
    }

    ret = mVidStream->uninit();
    if (ret) {
        log_error("Error in Video Stream uninit");
    }

    mVidStream.reset();

    return 0;
}

uint8_t CameraComponent::getVideoStreamStatus() const
{
    uint8_t ret = 0;

    if (!mVidStream)
        return 0;

    switch (mVidStream->getState()) {
    case VideoStream::STATE_ERROR:
    case VideoStream::STATE_IDLE:
    case VideoStream::STATE_INIT:
        ret = 0;
        break;
    case VideoStream::STATE_RUN:
        ret = 1;
        break;
    default:
        ret = 0;
        break;
    }

    log_debug("%s Status:%d", __func__, ret);
    return ret;
}

/* Input string can be either null-terminated or not */
std::string CameraComponent::toString(const char *buf, size_t buf_size)
{
    const char *end = std::find(buf, buf + buf_size, '\0');
    return std::string(buf, end);
}
