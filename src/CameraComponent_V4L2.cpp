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
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sstream>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "mavlink_server.h"

#include "CameraComponent_V4L2.h"
#include "log.h"

CameraComponent_V4L2::CameraComponent_V4L2()
    : CameraComponent()
{
}

CameraComponent_V4L2::CameraComponent_V4L2(std::string devicepath)
    : CameraComponent()
{
    log_debug("%s path:%s", __func__, devicepath.c_str());
}

CameraComponent_V4L2::CameraComponent_V4L2(std::string devicepath, std::string uri)
    : CameraComponent()
{

    log_debug("%s path:%s uri:%s", __func__, devicepath.c_str(), uri.c_str());
    // TODO:: get the supported paramters from the camera device
    // Initialize the supported and default values
    dev_path = devicepath;
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
    int fd = v4l2_open_device(dev_path.c_str());
    v4l2_device_info(fd);
    v4l2_close_device(fd);

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
}

void CameraComponent_V4L2::initSupportedValues()
{
    // TODO :: Get the details of the supported list from the camera device or from xml/conf file
}

void CameraComponent_V4L2::initDefaultValues()
{
    // TODO :: Get the details of the default values from the camera device and then save it in DB
    saveParameter(CameraParameters::CAMERA_MODE,
                  (uint32_t)CameraParameters::id_camera_mode::ID_CAMERA_MODE_VIDEO);
    saveParameter(CameraParameters::BRIGHTNESS, (uint32_t)56);
    saveParameter(CameraParameters::CONTRAST, (uint32_t)32);
    saveParameter(CameraParameters::SATURATION, (uint32_t)128);
    saveParameter(CameraParameters::HUE, (int32_t)0);
    saveParameter(CameraParameters::WHITE_BALANCE_MODE,
                  (uint32_t)CameraParameters::id_white_balance_mode::ID_WHITE_BALANCE_AUTO);
    saveParameter(CameraParameters::GAMMA, (uint32_t)220);
    saveParameter(CameraParameters::GAIN, (uint32_t)32);
    saveParameter(CameraParameters::POWER_LINE_FREQ_MODE, (uint32_t)0);
    saveParameter(CameraParameters::WHITE_BALANCE_TEMPERATURE, (uint32_t)6500);
    saveParameter(CameraParameters::SHARPNESS, (uint32_t)0);
    saveParameter(CameraParameters::BACKLIGHT_COMPENSATION, (uint32_t)1);
    saveParameter(CameraParameters::EXPOSURE_MODE, (uint32_t)3);
    saveParameter(CameraParameters::EXPOSURE_ABSOLUTE, (uint32_t)1);
    saveParameter(CameraParameters::VIDEO_SIZE,
                  (uint32_t)CameraParameters::id_video_size::ID_VIDEO_SIZE_640x480x30);

    // dummy values
    saveParameter(CameraParameters::IMAGE_SIZE,
                  CameraParameters::id_image_size::ID_IMAGE_SIZE_3264x2448);
    saveParameter(CameraParameters::IMAGE_FORMAT,
                  CameraParameters::id_image_format::ID_IMAGE_FORMAT_JPEG);
    saveParameter(CameraParameters::PIXEL_FORMAT,
                  CameraParameters::id_pixel_format::ID_PIXEL_FORMAT_YUV420P);
}

int CameraComponent_V4L2::getParam(const char *param_id, char *param_value)
{
    // TODO :: Do appropriate checks
    // query the value set in the map and fill the output, return appropriate value
    std::string value = camParam.getParameter(param_id);
    strcpy(param_value, value.c_str());
    return 1;
}

int CameraComponent_V4L2::setParam(const char *param_id, const char *param_value, int param_type)
{
    int ret = 1;
    mavlink_param_union_t u;
    memcpy(&u.param_float, param_value, sizeof(float));
    switch (param_type) {
    case CameraParameters::PARAM_TYPE_REAL32:
        setParam(param_id, u.param_float);
        break;
    case CameraParameters::PARAM_TYPE_INT32:
        setParam(param_id, u.param_int32);
        break;
    case CameraParameters::PARAM_TYPE_UINT32:
        setParam(param_id, u.param_uint32);
        break;
    case CameraParameters::PARAM_TYPE_UINT8:
        setParam(param_id, u.param_uint8);
        break;
    default:
        break;
    }

    return ret;
}

int CameraComponent_V4L2::setParam(const char *param_id, float param_value)
{
    log_debug("%s: Param Id:%s Value:%lf", __func__, param_id, param_value);
    return 0;
}

int CameraComponent_V4L2::setParam(const char *param_id, int32_t param_value)
{
    log_debug("%s: Param Id:%s Value:%d", __func__, param_id, param_value);
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
        int fd = v4l2_open_device(dev_path.c_str());
        ret = v4l2_set_control(fd, ctrl_id, param_value);
        if (ret)
            log_error("Error in setting control : %s Error:%d", param_id, errno);
        v4l2_close_device(fd);
        if (!ret)
            saveParameter(param_id, param_value);
    }
    return ret;
}

int CameraComponent_V4L2::setParam(const char *param_id, uint32_t param_value)
{
    // TODO :: Do appropriate checks
    // Check if camera is open for any usecase?
    // Check if param is available
    // Check if param is already set to same value
    // Check if value is supported
    log_debug("%s: Param Id:%s Value:%d", __func__, param_id, param_value);
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
        int fd = v4l2_open_device(dev_path.c_str());
        ret = v4l2_set_control(fd, ctrl_id, param_value);
        if (ret)
            log_error("Error in setting control : %s Error:%d", param_id, errno);
        v4l2_close_device(fd);
        if (!ret)
            saveParameter(param_id, param_value);
    }
    return ret;
}

int CameraComponent_V4L2::setParam(const char *param_id, uint8_t param_value)
{
    log_debug("%s: Param Id:%s Value:%d", __func__, param_id, param_value);
    return 0;
}

int CameraComponent_V4L2::setCameraMode(uint32_t param_value)
{
    return 0;
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

int CameraComponent_V4L2::saveParameter(const char *param_id, float param_value)
{
    char str[128];
    mavlink_param_union_t u;
    u.param_float = param_value;
    memcpy(&str[0], &u.param_float, sizeof(float));
    camParam.setParameter(param_id, str);
    return 0;
}

int CameraComponent_V4L2::saveParameter(const char *param_id, uint32_t param_value)
{
    char str[128];
    mavlink_param_union_t u;
    u.param_uint32 = param_value;
    memcpy(&str[0], &u.param_float, sizeof(float));
    camParam.setParameter(param_id, str);
    return 0;
}

int CameraComponent_V4L2::saveParameter(const char *param_id, int32_t param_value)
{
    char str[128];
    mavlink_param_union_t u;
    u.param_int32 = param_value;
    memcpy(&str[0], &u.param_float, sizeof(float));
    camParam.setParameter(param_id, str);
    return 0;
}

int CameraComponent_V4L2::saveParameter(const char *param_id, uint8_t param_value)
{
    char str[128];
    mavlink_param_union_t u;
    u.param_uint8 = param_value;
    memcpy(&str[0], &u.param_float, sizeof(float));
    camParam.setParameter(param_id, str);
    return 0;
}

/*
*
* V4L2 Interfaces
*
*/

int CameraComponent_V4L2::xioctl(int fd, int request, void *arg)
{
    int r;

    do
        r = ioctl(fd, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

int CameraComponent_V4L2::v4l2_open_device(const char *devicepath)
{
    int fd = -1;
    fd = open(devicepath, O_RDWR, 0);
    if (fd < 0) {
        log_error("Cannot open device '%s': %d: ", devicepath, errno);
    }

    return fd;
}

int CameraComponent_V4L2::v4l2_close_device(int fd)
{
    if (fd < 1)
        return -1;

    close(fd);
    return 0;
}

int CameraComponent_V4L2::v4l2_get_control(int fd, int ctrl_id)
{
    int ret = -1;
    if (fd < 1)
        return ret;

    struct v4l2_control ctrl = {0};
    ctrl.id = ctrl_id;
    ret = xioctl(fd, VIDIOC_G_CTRL, &ctrl);
    if (ret == 0)
        return ctrl.value;
    else
        return ret;
}

int CameraComponent_V4L2::v4l2_set_control(int fd, int ctrl_id, int value)
{
    int ret = -1;
    if (fd < 1)
        return ret;

    struct v4l2_control c = {0};
    c.id = ctrl_id;
    c.value = value;
    ret = xioctl(fd, VIDIOC_S_CTRL, &c);

    return ret;
}

int CameraComponent_V4L2::v4l2_query_control(int fd)
{
    int ret = -1;
    if (fd < 1)
        return ret;

    const unsigned next_fl = V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;
    struct v4l2_control ctrl = {0};
    struct v4l2_queryctrl qctrl = {0};
    qctrl.id = next_fl;
    while (xioctl(fd, VIDIOC_QUERYCTRL, &qctrl) == 0) {
        if ((qctrl.flags & V4L2_CTRL_TYPE_BOOLEAN) || (qctrl.type & V4L2_CTRL_TYPE_INTEGER)) {
            log_debug("Ctrl: %s Min:%d Max:%d Step:%d dflt:%d", qctrl.name, qctrl.minimum,
                      qctrl.maximum, qctrl.step, qctrl.default_value);
            ctrl.id = qctrl.id;
            if (xioctl(fd, VIDIOC_G_CTRL, &ctrl) == 0) {
                log_debug("Ctrl: %s Id:%x Value:%d\n", qctrl.name, ctrl.id, ctrl.value);
            }
        }
        qctrl.id |= next_fl;
    }
    return 0;
}

int CameraComponent_V4L2::v4l2_device_info(int fd)
{
    int ret = -1;
    if (fd < 1)
        return ret;

    struct v4l2_capability vcap;
    ret = xioctl(fd, VIDIOC_QUERYCAP, &vcap);
    if (ret)
        return ret;

    log_debug("\tDriver name   : %s", vcap.driver);
    log_debug("\tCard type     : %s", vcap.card);
    log_debug("\tBus info      : %s", vcap.bus_info);
    log_debug("\tDriver version: %d.%d.%d", vcap.version >> 16, (vcap.version >> 8) & 0xff,
              vcap.version & 0xff);
    log_debug("\tCapabilities  : 0x%08X", vcap.capabilities);
    if (vcap.capabilities & V4L2_CAP_DEVICE_CAPS) {
        log_debug("\tDevice Caps   : 0x%08X", vcap.device_caps);
    }

    return 0;
}
