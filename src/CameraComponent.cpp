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
#include "CameraComponent.h"
#include "CameraDeviceV4l2.h"
#include "ImageCaptureGst.h"
#include "mavlink_server.h"
#include "util.h"
#include <algorithm>
#ifdef ENABLE_GAZEBO
#include "CameraDeviceGazebo.h"
#endif

CameraComponent::CameraComponent(std::string camdev_name)
    : mCamDevName(camdev_name)
    , mCamInfo{}
    , mStoreInfo{}
    , mImgPath("")
{
    log_debug("%s path:%s", __func__, camdev_name.c_str());
    // Create a camera device based on device path
    mCamDev = create_camera_device(camdev_name);
    if (!mCamDev)
        return; // TODO :: Raise exception

    // Get info from the camera device
    mCamDev->getInfo(mCamInfo);
    // append uri-null info to the structure

    // Get list of Parameters supported & its default value
    mCamDev->init(mCamParam);

    // start the camera device
    mCamDev->start();

    initStorageInfo(mStoreInfo);
}

CameraComponent::CameraComponent(std::string camdev_name, std::string camdef_uri)
    : mCamDevName(camdev_name)
    , mCamInfo{}
    , mStoreInfo{}
    , mCamDefURI(camdef_uri)
    , mImgPath("")
{
    log_debug("%s path:%s", __func__, camdev_name.c_str());
    // Create a camera device based on device path
    mCamDev = create_camera_device(camdev_name);
    if (!mCamDev)
        return; // TODO :: Raise exception

    // Get info from the camera device
    mCamDev->getInfo(mCamInfo);

    if (sizeof(mCamInfo.cam_definition_uri) > mCamDefURI.size()) {
        strcpy((char *)mCamInfo.cam_definition_uri, mCamDefURI.c_str());
    } else {
        log_error("URI length bigger than permitted");
        // TODO::Continue with no parameter support
    }

    // Get list of Parameters supported & its default value
    mCamDev->init(mCamParam);

    // start the camera device
    mCamDev->start();

    initStorageInfo(mStoreInfo);
}

CameraComponent::~CameraComponent()
{
}

const CameraInfo &CameraComponent::getCameraInfo() const
{
    return mCamInfo;
}

const StorageInfo &CameraComponent::getStorageInfo() const
{
    return mStoreInfo;
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
    if (!value.empty())
        return 1;

    mem_cpy(param_value, value_size, value.data(), value.size(), value_size);
    return 0;
}

int CameraComponent::setParam(const char *param_id, size_t id_size, const char *param_value,
                              size_t value_size, int param_type)
{
    int ret = 1;
    CameraParameters::cam_param_union_t u;
    std::string id = toString(param_id, id_size);
    memcpy(&u.param_float, param_value, sizeof(float));
    switch (param_type) {
    case CameraParameters::PARAM_TYPE_REAL32:
        ret = setParam(id, u.param_float);
        break;
    case CameraParameters::PARAM_TYPE_INT32:
        ret = setParam(id, u.param_int32);
        break;
    case CameraParameters::PARAM_TYPE_UINT32:
        ret = setParam(id, u.param_uint32);
        break;
    case CameraParameters::PARAM_TYPE_UINT8:
        ret = setParam(id, u.param_uint8);
        break;
    default:
        ret = mCamDev->setParam(param_id, id_size, param_value,
                              value_size, param_type);
        break;
    }

    return ret;
}

int CameraComponent::setParam(std::string param_id, float param_value)
{
    int ret = 0;
    log_debug("%s: Param Id:%s Value:%lf", __func__, param_id.c_str(), param_value);
    ret = mCamDev->setParam(param_id, param_value);
    return ret;
}

int CameraComponent::setParam(std::string param_id, int32_t param_value)
{
    log_debug("%s: Param Id:%s Value:%d", __func__, param_id.c_str(), param_value);
    int ret = 0;
    int paramID = mCamParam.getParameterID(param_id);
    switch (paramID) {
    case CameraParameters::PARAM_ID_HUE:
        ret = mCamDev->setHue(param_value);
        break;
    default:
        ret = mCamDev->setParam(param_id, param_value);
        break;
    }

    if (!ret)
        mCamParam.setParameter(param_id, param_value);

    return ret;
}

int CameraComponent::setParam(std::string param_id, uint32_t param_value)
{
    // TODO :: Do appropriate checks
    // Check if camera is open for any usecase?
    // Check if param is available
    // Check if param is already set to same value
    // Check if value is supported
    log_debug("%s: Param Id:%s Value:%d", __func__, param_id.c_str(), param_value);
    int ret = 0;
    int paramID = mCamParam.getParameterID(param_id);
    switch (paramID) {
    case CameraParameters::PARAM_ID_CAMERA_MODE:
        ret = mCamDev->setMode(param_value);
        break;
    case CameraParameters::PARAM_ID_VIDEO_SIZE:
        ret = setVideoSize(param_value);
        break;
    case CameraParameters::PARAM_ID_VIDEO_FRAME_FORMAT:
        ret = setVideoFrameFormat(param_value);
        break;
    case CameraParameters::PARAM_ID_IMAGE_SIZE:
        ret = setImazeSize(param_value);
        break;
    case CameraParameters::PARAM_ID_IMAGE_FORMAT:
        ret = setImageFormat(param_value);
        break;
    case CameraParameters::PARAM_ID_BRIGHTNESS:
        ret = mCamDev->setBrightness(param_value);
        break;
    case CameraParameters::PARAM_ID_CONTRAST:
        ret = mCamDev->setContrast(param_value);
        break;
    case CameraParameters::PARAM_ID_SATURATION:
        ret = mCamDev->setSaturation(param_value);
        break;
    case CameraParameters::PARAM_ID_WHITE_BALANCE_MODE:
        ret = mCamDev->setWhiteBalanceMode(param_value);
        break;
    case CameraParameters::PARAM_ID_GAMMA:
        ret = mCamDev->setGamma(param_value);
        break;
    case CameraParameters::PARAM_ID_GAIN:
        ret = mCamDev->setGain(param_value);
        break;
    case CameraParameters::PARAM_ID_POWER_LINE_FREQ_MODE:
        ret = mCamDev->setPowerLineFrequency(param_value);
        break;
    case CameraParameters::PARAM_ID_WHITE_BALANCE_TEMPERATURE:
        ret = mCamDev->setWhiteBalanceTemperature(param_value);
        break;
    case CameraParameters::PARAM_ID_SHARPNESS:
        ret = mCamDev->setSharpness(param_value);
        break;
    case CameraParameters::PARAM_ID_BACKLIGHT_COMPENSATION:
        ret = mCamDev->setBacklightCompensation(param_value);
        break;
    case CameraParameters::PARAM_ID_EXPOSURE_MODE:
        ret = mCamDev->setExposureMode(param_value);
        break;
    case CameraParameters::PARAM_ID_EXPOSURE_ABSOLUTE:
        ret = mCamDev->setExposureAbsolute(param_value);
        break;
    case CameraParameters::PARAM_ID_PIXEL_FORMAT:
        ret = mCamDev->setPixelFormat(param_value);
        break;
    case CameraParameters::PARAM_ID_SCENE_MODE:
        ret = mCamDev->setSceneMode(param_value);
        break;
    default:
        ret = mCamDev->setParam(param_id, param_value);
        break;
    }

    if (!ret)
        mCamParam.setParameter(param_id, param_value);

    return ret;
}

int CameraComponent::setParam(std::string param_id, uint8_t param_value)
{
    int ret = 0;
    log_debug("%s: Param Id:%s Value:%d", __func__, param_id.c_str(), param_value);
    ret = mCamDev->setParam(param_id, param_value);
    return ret;
}

int CameraComponent::setCameraMode(uint32_t mode)
{
    mCamDev->setMode(mode);
    return 0;
}

int CameraComponent::getCameraMode()
{
    return mCamDev->getMode();
}

int CameraComponent::startImageCapture(int interval, int count, capture_callback_t cb)
{
    mImgCapCB = cb;

    // Delete imgCap instance if already exists
    // This could be because of no StopImageCapture call after done
    // Or new startImageCapture call while prev call is still not done
    if (mImgCap)
        mImgCap.reset();

    mImgCap = std::make_shared<ImageCaptureGst>(mCamDev);
    if (!mImgPath.empty())
        mImgCap->setLocation(mImgPath);
    mImgCap->start(interval, count, std::bind(&CameraComponent::cbImageCaptured, this,
                                              std::placeholders::_1, std::placeholders::_2));
    return 0;
}

int CameraComponent::stopImageCapture()
{
    if (mImgCap)
        mImgCap->stop();

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

int CameraComponent::setImageLocation(std::string imgPath)
{
    mImgPath = imgPath;
    return 0;
}

int CameraComponent::setImazeSize(uint32_t param_value)
{
    return 0;
}

int CameraComponent::setImageFormat(uint32_t param_value)
{
    return 0;
}

int CameraComponent::setVideoSize(uint32_t param_value)
{
    return 0;
}

int CameraComponent::setVideoFrameFormat(uint32_t param_value)
{
    return 0;
}

// TODO:: Move this operation to a factory class
std::shared_ptr<CameraDevice> CameraComponent::create_camera_device(std::string camdev_name)
{
    if (camdev_name.find("/dev/video") != std::string::npos) {
        log_debug("V4L2 device : %s", camdev_name.c_str());
        return std::make_shared<CameraDeviceV4l2>(camdev_name);
    } else if (camdev_name.find("camera/image") != std::string::npos) {
        log_debug("Gazebo device : %s", camdev_name.c_str());
#ifdef ENABLE_GAZEBO
        return std::make_shared<CameraDeviceGazebo>(camdev_name);
#else
        log_error("Gazebo device not supported");
        return nullptr;
#endif
    } else {
        log_error("Camera device not found");
        return nullptr;
    }
}

/* Input string can be either null-terminated or not */
std::string CameraComponent::toString(const char *buf, size_t buf_size)
{
    const char *end = std::find(buf, buf + buf_size, '\0');
    return std::string(buf, end);
}
