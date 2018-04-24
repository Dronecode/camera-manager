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

    cid = getV4l2ControlId(paramId);
    if (cid) {
        if (param_type == CameraParameters::PARAM_TYPE_INT32)
            ret = setV4l2Control(cid, u.param_int32);
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

int CameraDeviceV4l2::initParams(CameraParameters &camParam)
{
    int ret = 0;

    ret = declareV4l2Params(camParam);
    if (ret)
        log_error("Error in declaring v4l2 param");

    ret = declareParams(camParam);

    return ret;
}

int CameraDeviceV4l2::resetParams(CameraParameters &camParam)
{
    log_info("%s", __func__);

    int ret = 0;

    ret = resetV4l2Params(camParam);

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

int CameraDeviceV4l2::declareV4l2Params(CameraParameters &camParam)
{
    int ret = 0;
    int value;
    int camFd = v4l2_open(mDeviceId.c_str());

    const unsigned next_fl = V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;
    struct v4l2_queryctrl qctrl = {0};
    struct v4l2_control ctrl = {0};

    qctrl.id = next_fl;
    while (v4l2_ioctl(camFd, VIDIOC_QUERYCTRL, &qctrl) == 0) {
        ctrl.id = qctrl.id;
        if (v4l2_ioctl(camFd, VIDIOC_G_CTRL, &ctrl) == 0)
            value = ctrl.value;
        else
            value = qctrl.default_value;

        log_info("Add Param:%s Type:%d Value:%d", qctrl.name, qctrl.type, value);
        camParam.setParameterIdType(getParamName(qctrl.id), getParamId(qctrl.id),
                                    getParamType((v4l2_ctrl_type)qctrl.type));
        camParam.setParameter(getParamName(qctrl.id), (int32_t)value);

        qctrl.id |= next_fl;
    }

    v4l2_close(camFd);

    return ret;
}

int CameraDeviceV4l2::resetV4l2Params(CameraParameters &camParam)
{
    int ret = 0;
    int status = -1;
    int32_t value = 0;
    std::string param;

    int camFd = v4l2_open(mDeviceId.c_str());

    const unsigned next_fl = V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;
    struct v4l2_queryctrl qctrl = {0};

    qctrl.id = next_fl;
    while (v4l2_ioctl(camFd, VIDIOC_QUERYCTRL, &qctrl) == 0) {
        if (qctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
            log_warning("V4L2 Control %s ID:%d disabled", qctrl.name, qctrl.id);
            qctrl.id |= next_fl;
            continue;
        }

        param = getParamName(qctrl.id);
        if (param.empty()) {
            qctrl.id |= next_fl;
            continue;
        }

        value = (int32_t)qctrl.default_value;
        log_info("resetV4l2Params Id:%d Value:%d", qctrl.id, value);
        ret = v4l2_set_control(camFd, qctrl.id, value);
        if (!ret) {
            if (camParam.setParameter(param, (int32_t)value))
                status = 0;
            else
                log_error("Error in saving v4l2 param :%s", param.c_str());
        } else
            log_error("Error in setting %s to default", qctrl.name);

        qctrl.id |= next_fl;
    }

    v4l2_close(camFd);

    return status;
}

std::string CameraDeviceV4l2::getParamName(int cid)
{
    std::string param = {};

    switch (cid) {
    case V4L2_CID_BRIGHTNESS:
        param = CameraParameters::BRIGHTNESS;
        break;

    case V4L2_CID_CONTRAST:
        param = CameraParameters::CONTRAST;
        break;
    case V4L2_CID_SATURATION:
        param = CameraParameters::SATURATION;
        break;

    case V4L2_CID_AUTO_WHITE_BALANCE:
        param = CameraParameters::WHITE_BALANCE_MODE;
        break;

    case V4L2_CID_GAIN:
        param = CameraParameters::GAIN;
        break;

    case V4L2_CID_HUE:
        param = CameraParameters::HUE;
        break;

    case V4L2_CID_GAMMA:
        param = CameraParameters::GAMMA;
        break;

    case V4L2_CID_POWER_LINE_FREQUENCY:
        param = CameraParameters::POWER_LINE_FREQ_MODE;
        break;

    case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
        param = CameraParameters::WHITE_BALANCE_TEMPERATURE;
        break;

    case V4L2_CID_SHARPNESS:
        param = CameraParameters::SHARPNESS;
        break;

    case V4L2_CID_BACKLIGHT_COMPENSATION:
        param = CameraParameters::BACKLIGHT_COMPENSATION;
        break;

    case V4L2_CID_EXPOSURE_AUTO:
        param = CameraParameters::EXPOSURE_MODE;
        break;

    case V4L2_CID_EXPOSURE:
        param = CameraParameters::EXPOSURE;
        break;

    case V4L2_CID_EXPOSURE_ABSOLUTE:
        param = CameraParameters::EXPOSURE_ABSOLUTE;
        break;

    case V4L2_CID_EXPOSURE_AUTO_PRIORITY:
        param = CameraParameters::EXPOSURE_AUTO_PRIORITY;
        break;

    default:
        log_error("Unknown V4L2 Control Parameter ");
        break;
    }

    return param;
}

int CameraDeviceV4l2::getParamId(int cid)
{
    int paramId = -1;

    switch (cid) {
    case V4L2_CID_BRIGHTNESS:
        paramId = CameraParameters::PARAM_ID_BRIGHTNESS;
        break;

    case V4L2_CID_CONTRAST:
        paramId = CameraParameters::PARAM_ID_CONTRAST;
        break;
    case V4L2_CID_SATURATION:
        paramId = CameraParameters::PARAM_ID_SATURATION;
        break;

    case V4L2_CID_AUTO_WHITE_BALANCE:
        paramId = CameraParameters::PARAM_ID_WHITE_BALANCE_MODE;
        break;

    case V4L2_CID_GAIN:
        paramId = CameraParameters::PARAM_ID_GAIN;
        break;

    case V4L2_CID_HUE:
        paramId = CameraParameters::PARAM_ID_HUE;
        break;

    case V4L2_CID_GAMMA:
        paramId = CameraParameters::PARAM_ID_GAMMA;
        break;

    case V4L2_CID_POWER_LINE_FREQUENCY:
        paramId = CameraParameters::PARAM_ID_POWER_LINE_FREQ_MODE;
        break;

    case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
        paramId = CameraParameters::PARAM_ID_WHITE_BALANCE_TEMPERATURE;
        break;

    case V4L2_CID_SHARPNESS:
        paramId = CameraParameters::PARAM_ID_SHARPNESS;
        break;

    case V4L2_CID_BACKLIGHT_COMPENSATION:
        paramId = CameraParameters::PARAM_ID_BACKLIGHT_COMPENSATION;
        break;

    case V4L2_CID_EXPOSURE_AUTO:
        paramId = CameraParameters::PARAM_ID_EXPOSURE_MODE;
        break;

    case V4L2_CID_EXPOSURE:
        paramId = CameraParameters::PARAM_ID_EXPOSURE;
        break;

    case V4L2_CID_EXPOSURE_ABSOLUTE:
        paramId = CameraParameters::PARAM_ID_EXPOSURE_ABSOLUTE;
        break;

    case V4L2_CID_EXPOSURE_AUTO_PRIORITY:
        paramId = CameraParameters::PARAM_ID_EXPOSURE_AUTO_PRIORITY;
        break;

    default:
        log_error("Unknown V4L2 Control Parameter ");
        break;
    }

    return paramId;
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

int CameraDeviceV4l2::getV4l2ControlId(int paramId)
{
    int cid = -1;

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

    case CameraParameters::PARAM_ID_EXPOSURE:
        cid = V4L2_CID_EXPOSURE;
        break;

    case CameraParameters::PARAM_ID_EXPOSURE_ABSOLUTE:
        cid = V4L2_CID_EXPOSURE_ABSOLUTE;
        break;

    case CameraParameters::PARAM_ID_EXPOSURE_AUTO_PRIORITY:
        cid = V4L2_CID_EXPOSURE_AUTO_PRIORITY;
        break;

    default:
        log_error("Unknown Parameter Id:%d", paramId);
        break;
    }

    return cid;
}

int CameraDeviceV4l2::setV4l2Control(int ctrl_id, int value)
{
    int fd = v4l2_open(mDeviceId.c_str());
    int ret = v4l2_set_control(fd, ctrl_id, value);
    if (ret)
        log_error("Error in setting control : %d Error:%d", ctrl_id, errno);
    v4l2_close(fd);
    return ret;
}
