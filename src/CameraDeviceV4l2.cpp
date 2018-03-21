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
#include <cstring>
#include <linux/videodev2.h>
#include <sys/ioctl.h>

#include "CameraDeviceV4l2.h"
#include "v4l2_interface.h"

CameraDeviceV4l2::CameraDeviceV4l2(std::string device)
    : mDeviceId(device)
    , mCardName("v4l2-card")
    , mDriverName("v4l2-drv")
    , mMode(-1)
{
    log_info("%s Node: %s", __func__, mDeviceId.c_str());
    int ret = initInfo();
    if (ret)
        log_error("Error in reading camera info");
}

CameraDeviceV4l2::~CameraDeviceV4l2()
{
}

int CameraDeviceV4l2::init(CameraParameters &camParam)
{
    // TODO::Query supported parameters
    // TODO::Set default parameters set
    initParams(camParam);
    return 0;
}

int CameraDeviceV4l2::uninit()
{
    return 0;
}

int CameraDeviceV4l2::start()
{
    return 0;
}

int CameraDeviceV4l2::stop()
{
    return 0;
}

std::vector<uint8_t> CameraDeviceV4l2::read()
{
    return std::vector<uint8_t>();
}

int CameraDeviceV4l2::setParam(CameraParameters &camParam, std::string param,
                               const char *param_value, size_t value_size, int param_type)

{
    int ret = 0;
    int cid = 0;
    CameraParameters::cam_param_union_t u;
    memcpy(&u.param_float, param_value, sizeof(float));
    int paramId = camParam.getParameterID(param);
    log_info("Parameter: %s Value: %d", param.c_str(), u.param_int32);
    switch (paramId) {
    case CameraParameters::PARAM_ID_BRIGHTNESS:
        cid = V4L2_CID_BRIGHTNESS;
        break;
    case CameraParameters::PARAM_ID_CONTRAST:
        cid = V4L2_CID_CONTRAST;
        break;
    case CameraParameters::PARAM_ID_SATURATION:
        cid = V4L2_CID_SATURATION;
        break;
    case CameraParameters::PARAM_ID_WHITE_BALANCE_MODE:
        cid = V4L2_CID_AUTO_WHITE_BALANCE;
        break;

    case CameraParameters::PARAM_ID_GAIN:
        cid = V4L2_CID_GAIN;
        break;

    case CameraParameters::PARAM_ID_HUE:
        cid = V4L2_CID_HUE;
        break;

    case CameraParameters::PARAM_ID_GAMMA:
        cid = V4L2_CID_GAMMA;
        break;

    case CameraParameters::PARAM_ID_POWER_LINE_FREQ_MODE:
        cid = V4L2_CID_POWER_LINE_FREQUENCY;
        break;

    case CameraParameters::PARAM_ID_WHITE_BALANCE_TEMPERATURE:
        cid = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
        break;

    case CameraParameters::PARAM_ID_SHARPNESS:
        cid = V4L2_CID_SHARPNESS;
        break;

    case CameraParameters::PARAM_ID_BACKLIGHT_COMPENSATION:
        cid = V4L2_CID_BACKLIGHT_COMPENSATION;
        break;

    case CameraParameters::PARAM_ID_EXPOSURE_MODE:
        cid = V4L2_CID_EXPOSURE_AUTO;
        break;

    case CameraParameters::PARAM_ID_EXPOSURE_ABSOLUTE:
        cid = V4L2_CID_EXPOSURE_ABSOLUTE;
        break;

    case CameraParameters::PARAM_ID_EXPOSURE_AUTO_PRIORITY:
        cid = V4L2_CID_EXPOSURE_AUTO_PRIORITY;
        break;

    default:
        log_error("Unknown Parameter : %s Id:%d", param.c_str(), paramId);
        break;
    }

    if (cid) {
        if (param_type == CameraParameters::PARAM_TYPE_INT32)
            ret = set_control(cid, u.param_int32);
        else
            log_error("Unhandled Parameter Type");
    }

    if (!ret)
        camParam.setParameter(param, u.param_int32);

    return ret;
}

std::string CameraDeviceV4l2::getDeviceId()
{
    return mDeviceId;
}

int CameraDeviceV4l2::initInfo()
{
    int ret = 0;
    struct v4l2_capability vCap;
    int camFd = v4l2_open(mDeviceId.c_str());
    if (camFd < 0) {
        log_error("Error in opening camera device");
        return -1;
    }

    ret = v4l2_query_cap(camFd, vCap);
    if (ret) {
        log_error("Error in Query Capability : Error:%d", errno);
    } else {
        mCardName = std::string(reinterpret_cast<char *>(vCap.card));
        mDriverName = std::string(reinterpret_cast<char *>(vCap.driver));
    }

    mVersion = vCap.version;

    log_info("card = %s driver = %s", mCardName.c_str(), mDriverName.c_str());
    log_debug("Kernel version = %d", mVersion);

    v4l2_close(camFd);

    // TODO:: Info not available from v4l2 IOCTLs can be read from xml file

    return ret;
}

int CameraDeviceV4l2::getInfo(struct CameraInfo &camInfo)
{
    strncpy((char *)camInfo.vendorName, mCardName.c_str(), sizeof(camInfo.vendorName));
    camInfo.vendorName[sizeof(camInfo.vendorName) - 1] = 0;
    strncpy((char *)camInfo.modelName, mDriverName.c_str(), sizeof(camInfo.modelName));
    camInfo.modelName[sizeof(camInfo.modelName) - 1] = 0;
    camInfo.firmware_version = mVersion;
    camInfo.focal_length = 0;
    camInfo.sensor_size_h = 0;
    camInfo.sensor_size_v = 0;
    camInfo.resolution_h = 0;
    camInfo.resolution_v = 0;
    camInfo.lens_id = 0;
    camInfo.flags = ~0u; // TODO :: Replace with flags
    camInfo.cam_definition_version = 1;
    return 0;
}

bool CameraDeviceV4l2::isGstV4l2Src()
{
    return true;
}

int CameraDeviceV4l2::setSize(uint32_t width, uint32_t height)
{
    return 0;
}

int CameraDeviceV4l2::setPixelFormat(uint32_t format)
{
    return 0;
}

int CameraDeviceV4l2::setMode(uint32_t mode)
{
    return 0;
}

int CameraDeviceV4l2::getMode()
{
    return mMode;
}

int CameraDeviceV4l2::resetParams(CameraParameters &camParam)
{
    int ret = 0;
    struct v4l2_queryctrl queryctrl;

    int fd = v4l2_open(mDeviceId.c_str());

    if (fd == -1) {
        log_debug("Error in opening camera device in resetParams()");
        ret = 1;
        return ret;
    }

    memset(&queryctrl, 0, sizeof(queryctrl));

    for (queryctrl.id = V4L2_CID_BASE; queryctrl.id < V4L2_CID_LASTP1; queryctrl.id++) {
        if (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl)) {
            if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                continue;
        }
        ret = v4l2_set_control(fd, queryctrl.id, queryctrl.default_value);
        //    log_debug("return value for:%s : %d : %d",queryctrl.name,queryctrl.id,ret);

        switch (queryctrl.id) {
        case V4L2_CID_BRIGHTNESS:
            camParam.setParameter(CameraParameters::BRIGHTNESS, (uint32_t)queryctrl.default_value);
            break;
        case V4L2_CID_CONTRAST:
            camParam.setParameter(CameraParameters::CONTRAST, (uint32_t)queryctrl.default_value);
            break;
        case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
            camParam.setParameter(CameraParameters::WHITE_BALANCE_TEMPERATURE,
                                  (uint32_t)queryctrl.default_value);
            break;
        case V4L2_CID_SATURATION:
            camParam.setParameter(CameraParameters::SATURATION, (uint32_t)queryctrl.default_value);
            break;
        case V4L2_CID_HUE:
            camParam.setParameter(CameraParameters::HUE, (int32_t)queryctrl.default_value);
            break;
        case V4L2_CID_EXPOSURE:
            camParam.setParameter(CameraParameters::EXPOSURE_ABSOLUTE,
                                  (uint32_t)queryctrl.default_value);
            break;
        case V4L2_CID_GAIN:
            camParam.setParameter(CameraParameters::GAIN, (uint32_t)queryctrl.default_value);
            break;
        case V4L2_CID_POWER_LINE_FREQUENCY:
            camParam.setParameter(CameraParameters::POWER_LINE_FREQ_MODE,
                                  (uint32_t)queryctrl.default_value);
            break;
        case V4L2_CID_SHARPNESS:
            camParam.setParameter(CameraParameters::SHARPNESS, (uint32_t)queryctrl.default_value);
            break;
            //     case V4L2_CID_AUTOGAIN:
            //     case V4L2_CID_HFLIP:
            //     case V4L2_CID_VFLIP:
        }
    }
    //    log_debug("Reset Done!");
    return ret;
}

CameraParameters::param_type CameraDeviceV4l2::getParamType(v4l2_ctrl_type type)
{
    CameraParameters::param_type ret;
    switch (type) {
    case V4L2_CTRL_TYPE_INTEGER:
    case V4L2_CTRL_TYPE_BOOLEAN:
    case V4L2_CTRL_TYPE_MENU:
        ret = CameraParameters::PARAM_TYPE_INT32;
        break;
    case V4L2_CTRL_TYPE_INTEGER64:
        ret = CameraParameters::PARAM_TYPE_INT64;
        break;
    case V4L2_CTRL_TYPE_U8:
        ret = CameraParameters::PARAM_TYPE_UINT8;
        break;
    case V4L2_CTRL_TYPE_U16:
        ret = CameraParameters::PARAM_TYPE_UINT16;
        break;
    case V4L2_CTRL_TYPE_U32:
        ret = CameraParameters::PARAM_TYPE_UINT32;
        break;
    case V4L2_CTRL_TYPE_BUTTON:
    case V4L2_CTRL_TYPE_CTRL_CLASS:
    case V4L2_CTRL_TYPE_STRING:
    case V4L2_CTRL_TYPE_BITMASK:
    case V4L2_CTRL_TYPE_INTEGER_MENU:
    default:
        ret = CameraParameters::PARAM_TYPE_UINT32;
        break;
    }

    return ret;
}

int CameraDeviceV4l2::declareParams(CameraParameters &camParam)
{
    int ret = 0;

    camParam.setParameterIdType(CameraParameters::CAMERA_MODE,
                                CameraParameters::PARAM_ID_CAMERA_MODE,
                                CameraParameters::PARAM_TYPE_UINT32);
    camParam.setParameter(CameraParameters::CAMERA_MODE,
                          (uint32_t)CameraParameters::ID_CAMERA_MODE_VIDEO);

    return ret;
}

int CameraDeviceV4l2::declareV4l2Params(CameraParameters &camParam,
                                        struct v4l2_query_ext_ctrl &qctrl, int32_t value)
{
    int ret = 0;
    log_info("Add Param:%s Type:%d Value:%d", qctrl.name, qctrl.type, value);

    switch (qctrl.id) {
    case V4L2_CID_BRIGHTNESS:
        camParam.setParameterIdType(CameraParameters::BRIGHTNESS,
                                    CameraParameters::PARAM_ID_BRIGHTNESS,
                                    getParamType((v4l2_ctrl_type)qctrl.type));
        camParam.setParameter(CameraParameters::BRIGHTNESS, (int32_t)value);
        break;
    case V4L2_CID_CONTRAST:
        camParam.setParameterIdType(CameraParameters::CONTRAST, CameraParameters::PARAM_ID_CONTRAST,
                                    getParamType((v4l2_ctrl_type)qctrl.type));
        camParam.setParameter(CameraParameters::CONTRAST, (int32_t)value);
        break;
    case V4L2_CID_SATURATION:
        camParam.setParameterIdType(CameraParameters::SATURATION,
                                    CameraParameters::PARAM_ID_SATURATION,
                                    getParamType((v4l2_ctrl_type)qctrl.type));
        camParam.setParameter(CameraParameters::SATURATION, (int32_t)value);
        break;

    case V4L2_CID_AUTO_WHITE_BALANCE:
        camParam.setParameterIdType(CameraParameters::WHITE_BALANCE_MODE,
                                    CameraParameters::PARAM_ID_WHITE_BALANCE_MODE,
                                    getParamType((v4l2_ctrl_type)qctrl.type));
        camParam.setParameter(CameraParameters::WHITE_BALANCE_MODE, (int32_t)value);
        break;

    case V4L2_CID_GAIN:
        camParam.setParameterIdType(CameraParameters::GAIN, CameraParameters::PARAM_ID_GAIN,
                                    getParamType((v4l2_ctrl_type)qctrl.type));
        camParam.setParameter(CameraParameters::GAIN, (int32_t)value);
        break;

    case V4L2_CID_HUE:
        camParam.setParameterIdType(CameraParameters::HUE, CameraParameters::PARAM_ID_HUE,
                                    getParamType((v4l2_ctrl_type)qctrl.type));
        camParam.setParameter(CameraParameters::HUE, (int32_t)value);
        break;

    case V4L2_CID_GAMMA:
        camParam.setParameterIdType(CameraParameters::GAMMA, CameraParameters::PARAM_ID_GAMMA,
                                    getParamType((v4l2_ctrl_type)qctrl.type));
        camParam.setParameter(CameraParameters::GAMMA, (int32_t)value);
        break;

    case V4L2_CID_POWER_LINE_FREQUENCY:
        camParam.setParameterIdType(CameraParameters::POWER_LINE_FREQ_MODE,
                                    CameraParameters::PARAM_ID_POWER_LINE_FREQ_MODE,
                                    getParamType((v4l2_ctrl_type)qctrl.type));
        camParam.setParameter(CameraParameters::POWER_LINE_FREQ_MODE, (int32_t)value);
        break;

    case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
        camParam.setParameterIdType(CameraParameters::WHITE_BALANCE_TEMPERATURE,
                                    CameraParameters::PARAM_ID_WHITE_BALANCE_TEMPERATURE,
                                    getParamType((v4l2_ctrl_type)qctrl.type));
        camParam.setParameter(CameraParameters::WHITE_BALANCE_TEMPERATURE, (int32_t)value);
        break;

    case V4L2_CID_SHARPNESS:
        camParam.setParameterIdType(CameraParameters::SHARPNESS,
                                    CameraParameters::PARAM_ID_SHARPNESS,
                                    getParamType((v4l2_ctrl_type)qctrl.type));
        camParam.setParameter(CameraParameters::SHARPNESS, (int32_t)value);
        break;

    case V4L2_CID_BACKLIGHT_COMPENSATION:
        camParam.setParameterIdType(CameraParameters::BACKLIGHT_COMPENSATION,
                                    CameraParameters::PARAM_ID_BACKLIGHT_COMPENSATION,
                                    getParamType((v4l2_ctrl_type)qctrl.type));
        camParam.setParameter(CameraParameters::BACKLIGHT_COMPENSATION, (int32_t)value);
        break;

    case V4L2_CID_EXPOSURE_AUTO:
        camParam.setParameterIdType(CameraParameters::EXPOSURE_MODE,
                                    CameraParameters::PARAM_ID_EXPOSURE_MODE,
                                    getParamType((v4l2_ctrl_type)qctrl.type));
        camParam.setParameter(CameraParameters::EXPOSURE_MODE, (int32_t)value);
        break;

    case V4L2_CID_EXPOSURE_ABSOLUTE:
        camParam.setParameterIdType(CameraParameters::EXPOSURE_ABSOLUTE,
                                    CameraParameters::PARAM_ID_EXPOSURE_ABSOLUTE,
                                    getParamType((v4l2_ctrl_type)qctrl.type));
        camParam.setParameter(CameraParameters::EXPOSURE_ABSOLUTE, (int32_t)value);
        break;

    case V4L2_CID_EXPOSURE_AUTO_PRIORITY:
        camParam.setParameterIdType(CameraParameters::EXPOSURE_AUTO_PRIORITY,
                                    CameraParameters::PARAM_ID_EXPOSURE_AUTO_PRIORITY,
                                    getParamType((v4l2_ctrl_type)qctrl.type));
        camParam.setParameter(CameraParameters::EXPOSURE_AUTO_PRIORITY, (int32_t)value);
        break;

    default:
        log_error("Unknown V4L2 Control Parameter ");
        ret = -1;
        break;
    }

    return ret;
}

int CameraDeviceV4l2::initParams(CameraParameters &camParam)
{
    int ret = 0;
    int32_t value;
    int camFd = v4l2_open(mDeviceId.c_str());

    const unsigned next_fl = V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;
    struct v4l2_query_ext_ctrl qctrl = {0};
    struct v4l2_control ctrl = {0};

    qctrl.id = next_fl;
    while (v4l2_ioctl(camFd, VIDIOC_QUERYCTRL, &qctrl) == 0) {
        ctrl.id = qctrl.id;
        if (v4l2_ioctl(camFd, VIDIOC_G_CTRL, &ctrl) == 0)
            value = ctrl.value;
        else
            value = qctrl.default_value;
        ret = declareV4l2Params(camParam, qctrl, value);
        if (ret)
            log_error("Error in declaring v4l2 param :%s", qctrl.name);
        qctrl.id |= next_fl;
    }

    v4l2_close(camFd);

    ret = declareParams(camParam);

    return ret;
}

int CameraDeviceV4l2::set_control(int ctrl_id, int value)
{
    int fd = v4l2_open(mDeviceId.c_str());
    int ret = v4l2_set_control(fd, ctrl_id, value);
    if (ret)
        log_error("Error in setting control : %d Error:%d", ctrl_id, errno);
    v4l2_close(fd);
    return ret;
}
