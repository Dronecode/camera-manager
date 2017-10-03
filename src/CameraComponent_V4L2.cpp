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
#include <assert.h>
#include <linux/videodev2.h>

#include "mavlink_server.h"

#include "CameraComponent_V4L2.h"
#include "log.h"
#include "v4l2_interface.h"

CameraComponent_V4L2::CameraComponent_V4L2()
    : CameraComponent()
{
}

CameraComponent_V4L2::CameraComponent_V4L2(std::string devicepath)
    : CameraComponent()
    , dev_path(devicepath)
{
    log_debug("%s path:%s", __func__, devicepath.c_str());
}

CameraComponent_V4L2::CameraComponent_V4L2(std::string devicepath, std::string uri)
    : CameraComponent()
    , dev_path(devicepath)
{

    log_debug("%s path:%s uri:%s", __func__, devicepath.c_str(), uri.c_str());
    // TODO:: get the supported parameters from the camera device
    // Initialize the supported and default values
    if (sizeof(camInfo.cam_definition_uri) < uri.size() + 1) {
        log_error("URI length bigger than permitted");
        // TODO::Continue with no parameter support
    } else
        strcpy((char *)camInfo.cam_definition_uri, uri.c_str());

    initCameraInfo();
    initSupportedValues();
    initDefaultValues();
}

CameraComponent_V4L2::~CameraComponent_V4L2()
{
}

void CameraComponent_V4L2::initCameraInfo()
{
    // TODO :: Get the details from the camera device
    // Or read it from conf file
    int fd = v4l2_open(dev_path.c_str());
    v4l2_query_cap(fd);
    v4l2_close(fd);

    strcpy((char *)camInfo.vendorName, "Intel");
    strcpy((char *)camInfo.modelName, "RealSense R200");
    camInfo.firmware_version = 1;
    camInfo.focal_length = 0;
    camInfo.sensor_size_h = 0;
    camInfo.sensor_size_v = 0;
    camInfo.resolution_h = 0;
    camInfo.resolution_v = 0;
    camInfo.lens_id = 0;
    camInfo.flags = ~0u;
    camInfo.cam_definition_version = 1;
}

void CameraComponent_V4L2::initStorageInfo()
{
    // TODO:: Fill storage details with values
    storeInfo.storage_id = 1;
    storeInfo.storage_count = 1;
    storeInfo.status = 2; /*formatted*/
    storeInfo.total_capacity = 50.0;
    storeInfo.used_capacity = 0.0;
    storeInfo.available_capacity = 50.0;
    storeInfo.read_speed = 128;
    storeInfo.write_speed = 128;
}

void CameraComponent_V4L2::initSupportedValues()
{
    // TODO :: Get the details of the supported list from the camera device or from xml/conf file
}

void CameraComponent_V4L2::initDefaultValues()
{
    // TODO :: Get the details of the default values from the camera device and then save it in DB
    saveParameter(CameraParameters::CAMERA_MODE, (uint32_t)CameraParameters::ID_CAMERA_MODE_VIDEO);
    saveParameter(CameraParameters::BRIGHTNESS, (uint32_t)56);
    saveParameter(CameraParameters::CONTRAST, (uint32_t)32);
    saveParameter(CameraParameters::SATURATION, (uint32_t)128);
    saveParameter(CameraParameters::HUE, (int32_t)0);
    saveParameter(CameraParameters::WHITE_BALANCE_MODE,
                  (uint32_t)CameraParameters::ID_WHITE_BALANCE_AUTO);
    saveParameter(CameraParameters::GAMMA, (uint32_t)220);
    saveParameter(CameraParameters::GAIN, (uint32_t)32);
    saveParameter(CameraParameters::POWER_LINE_FREQ_MODE, (uint32_t)0);
    saveParameter(CameraParameters::WHITE_BALANCE_TEMPERATURE, (uint32_t)6500);
    saveParameter(CameraParameters::SHARPNESS, (uint32_t)0);
    saveParameter(CameraParameters::BACKLIGHT_COMPENSATION, (uint32_t)1);
    saveParameter(CameraParameters::EXPOSURE_MODE, (uint32_t)3);
    saveParameter(CameraParameters::EXPOSURE_ABSOLUTE, (uint32_t)1);
    saveParameter(CameraParameters::VIDEO_SIZE,
                  (uint32_t)CameraParameters::ID_VIDEO_SIZE_640x480x30);

    // dummy values
    saveParameter(CameraParameters::IMAGE_SIZE, CameraParameters::ID_IMAGE_SIZE_3264x2448);
    saveParameter(CameraParameters::IMAGE_FORMAT, CameraParameters::ID_IMAGE_FORMAT_JPEG);
    saveParameter(CameraParameters::PIXEL_FORMAT, CameraParameters::ID_PIXEL_FORMAT_YUV420P);
}

int CameraComponent_V4L2::getParam(const char *param_id, char *param_value, size_t value_size)
{
    // TODO :: Do appropriate checks
    // query the value set in the map and fill the output, return appropriate value
    std::string value = camParam.getParameter(param_id);
    if (!value.empty())
        return 1;

    if (value_size < value.size())
        strncpy(param_value, value.c_str(), value_size);
    else
        strcpy(param_value, value.c_str());

    return 0;
}

int CameraComponent_V4L2::setParam(const char *param_id, const char *param_value, size_t value_size,
                                   int param_type)
{
    int ret = 1;
    mavlink_param_union_t u;
    memcpy(&u.param_float, param_value, sizeof(float));
    switch (param_type) {
    case CameraParameters::PARAM_TYPE_REAL32:
        ret = setParam(param_id, u.param_float);
        break;
    case CameraParameters::PARAM_TYPE_INT32:
        ret = setParam(param_id, u.param_int32);
        break;
    case CameraParameters::PARAM_TYPE_UINT32:
        ret = setParam(param_id, u.param_uint32);
        break;
    case CameraParameters::PARAM_TYPE_UINT8:
        ret = setParam(param_id, u.param_uint8);
        break;
    default:
        break;
    }

    return ret;
}

int CameraComponent_V4L2::setParam(std::string param_id, float param_value)
{
    log_debug("%s: Param Id:%s Value:%lf", __func__, param_id.c_str(), param_value);
    return 0;
}

int CameraComponent_V4L2::setParam(std::string param_id, int32_t param_value)
{
    log_debug("%s: Param Id:%s Value:%d", __func__, param_id.c_str(), param_value);
    int ret = 0;
    int ctrl_id = -1;
    int paramID = camParam.getParameterID(param_id);
    switch (paramID) {
    case CameraParameters::PARAM_ID_HUE:
        ctrl_id = V4L2_CID_HUE;
        break;
    default:
        break;
    }

    if (ctrl_id != -1) {
        int fd = v4l2_open(dev_path.c_str());
        ret = v4l2_set_control(fd, ctrl_id, param_value);
        if (ret)
            log_error("Error in setting control : %s Error:%d", param_id.c_str(), errno);
        v4l2_close(fd);
        if (!ret)
            saveParameter(param_id, param_value);
    }
    return ret;
}

int CameraComponent_V4L2::setParam(std::string param_id, uint32_t param_value)
{
    // TODO :: Do appropriate checks
    // Check if camera is open for any usecase?
    // Check if param is available
    // Check if param is already set to same value
    // Check if value is supported
    log_debug("%s: Param Id:%s Value:%d", __func__, param_id.c_str(), param_value);
    int ret = 0;
    int ctrl_id = -1;
    int paramID = camParam.getParameterID(param_id);
    switch (paramID) {
    case CameraParameters::PARAM_ID_CAMERA_MODE:
        ret = setCameraMode(param_value);
        break;
    case CameraParameters::PARAM_ID_BRIGHTNESS:
        ctrl_id = V4L2_CID_BRIGHTNESS;
        break;
    case CameraParameters::PARAM_ID_CONTRAST:
        ctrl_id = V4L2_CID_CONTRAST;
        break;
    case CameraParameters::PARAM_ID_SATURATION:
        ctrl_id = V4L2_CID_SATURATION;
        break;
    case CameraParameters::PARAM_ID_WHITE_BALANCE_MODE:
        ctrl_id = V4L2_CID_AUTO_WHITE_BALANCE;
        break;
    case CameraParameters::PARAM_ID_GAMMA:
        ctrl_id = V4L2_CID_GAMMA;
        break;
    case CameraParameters::PARAM_ID_GAIN:
        ctrl_id = V4L2_CID_GAIN;
        break;
    case CameraParameters::PARAM_ID_POWER_LINE_FREQ_MODE:
        ctrl_id = V4L2_CID_POWER_LINE_FREQUENCY;
        break;
    case CameraParameters::PARAM_ID_WHITE_BALANCE_TEMPERATURE:
        ctrl_id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
        break;
    case CameraParameters::PARAM_ID_SHARPNESS:
        ctrl_id = V4L2_CID_SHARPNESS;
        break;
    case CameraParameters::PARAM_ID_BACKLIGHT_COMPENSATION:
        ctrl_id = V4L2_CID_BACKLIGHT_COMPENSATION;
        break;
    case CameraParameters::PARAM_ID_EXPOSURE_MODE:
        ctrl_id = V4L2_CID_EXPOSURE_AUTO;
        break;
    case CameraParameters::PARAM_ID_EXPOSURE_ABSOLUTE:
        ctrl_id = V4L2_CID_EXPOSURE_ABSOLUTE;
        break;
    case CameraParameters::PARAM_ID_IMAGE_SIZE:
        ret = setImazeSize(param_value);
        break;
    case CameraParameters::PARAM_ID_IMAGE_FORMAT:
        ret = setImageFormat(param_value);
        break;
    case CameraParameters::PARAM_ID_PIXEL_FORMAT:
        ret = setPixelFormat(param_value);
        break;
    case CameraParameters::PARAM_ID_SCENE_MODE:
        ret = setSceneMode(param_value);
        break;
    case CameraParameters::PARAM_ID_VIDEO_SIZE:
        ret = setVideoSize(param_value);
        break;
    case CameraParameters::PARAM_ID_VIDEO_FRAME_FORMAT:
        ret = setVideoFrameFormat(param_value);
        break;
    default:
        ret = -1;
        break;
    }

    if (ctrl_id != -1) {
        int fd = v4l2_open(dev_path.c_str());
        ret = v4l2_set_control(fd, ctrl_id, param_value);
        if (ret)
            log_error("Error in setting control : %s Error:%d", param_id.c_str(), errno);
        v4l2_close(fd);
        if (!ret)
            saveParameter(param_id, param_value);
    }
    return ret;
}

int CameraComponent_V4L2::setParam(std::string param_id, uint8_t param_value)
{
    log_debug("%s: Param Id:%s Value:%d", __func__, param_id.c_str(), param_value);
    return 0;
}

int CameraComponent_V4L2::setCameraMode(uint32_t mode)
{
    mCamMode = mode;
    return 0;
}

int CameraComponent_V4L2::getCameraMode()
{
    return mCamMode;
}

int CameraComponent_V4L2::setImazeSize(uint32_t param_value)
{
    return 0;
}

int CameraComponent_V4L2::setImageFormat(uint32_t param_value)
{
    return 0;
}

int CameraComponent_V4L2::setPixelFormat(uint32_t param_value)
{
    return 0;
}

int CameraComponent_V4L2::setSceneMode(uint32_t param_value)
{
    return 0;
}

int CameraComponent_V4L2::setVideoSize(uint32_t param_value)
{
    return 0;
}

int CameraComponent_V4L2::setVideoFrameFormat(uint32_t param_value)
{
    return 0;
}

bool CameraComponent_V4L2::saveParameter(std::string param_id, float param_value)
{
    char str[128];
    mavlink_param_union_t u;
    u.param_float = param_value;
    memcpy(&str[0], &u.param_float, sizeof(float));
    std::string str2(str, sizeof(str));
    return camParam.setParameter(param_id, str2);
}

bool CameraComponent_V4L2::saveParameter(std::string param_id, uint32_t param_value)
{
    char str[128];
    mavlink_param_union_t u;
    u.param_uint32 = param_value;
    memcpy(&str[0], &u.param_uint32, sizeof(uint32_t));
    std::string str2(str, sizeof(str));
    return camParam.setParameter(param_id, str2);
}

bool CameraComponent_V4L2::saveParameter(std::string param_id, int32_t param_value)
{
    char str[128];
    mavlink_param_union_t u;
    u.param_int32 = param_value;
    memcpy(&str[0], &u.param_int32, sizeof(int32_t));
    std::string str2(str, sizeof(str));
    return camParam.setParameter(param_id, str2);
}

bool CameraComponent_V4L2::saveParameter(std::string param_id, uint8_t param_value)
{
    char str[128];
    mavlink_param_union_t u;
    u.param_uint8 = param_value;
    memcpy(&str[0], &u.param_uint8, sizeof(uint8_t));
    std::string str2(str, sizeof(str));
    return camParam.setParameter(param_id, str2);
}
