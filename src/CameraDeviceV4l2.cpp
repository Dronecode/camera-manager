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
    , mMode(-1)
{
}

CameraDeviceV4l2::~CameraDeviceV4l2()
{
}

int CameraDeviceV4l2::init(CameraParameters &camParam)
{
    // TODO::Query supported parameters
    // TODO::Set default parameters set
    mMode = CameraParameters::ID_CAMERA_MODE_VIDEO;
    camParam.setParameter(CameraParameters::CAMERA_MODE,
                          (uint32_t)CameraParameters::ID_CAMERA_MODE_VIDEO);
    camParam.setParameter(CameraParameters::BRIGHTNESS, (uint32_t)56);
    camParam.setParameter(CameraParameters::CONTRAST, (uint32_t)32);
    camParam.setParameter(CameraParameters::SATURATION, (uint32_t)128);
    camParam.setParameter(CameraParameters::HUE, (int32_t)0);
    camParam.setParameter(CameraParameters::WHITE_BALANCE_MODE,
                          (uint32_t)CameraParameters::ID_WHITE_BALANCE_AUTO);
    camParam.setParameter(CameraParameters::GAMMA, (uint32_t)220);
    camParam.setParameter(CameraParameters::GAIN, (uint32_t)32);
    camParam.setParameter(CameraParameters::POWER_LINE_FREQ_MODE, (uint32_t)0);
    camParam.setParameter(CameraParameters::WHITE_BALANCE_TEMPERATURE, (uint32_t)6500);
    camParam.setParameter(CameraParameters::SHARPNESS, (uint32_t)0);
    camParam.setParameter(CameraParameters::BACKLIGHT_COMPENSATION, (uint32_t)1);
    camParam.setParameter(CameraParameters::EXPOSURE_MODE, (uint32_t)3);
    camParam.setParameter(CameraParameters::EXPOSURE_ABSOLUTE, (uint32_t)1);
    camParam.setParameter(CameraParameters::VIDEO_SIZE,
                          (uint32_t)CameraParameters::ID_VIDEO_SIZE_640x480x30);

    // dummy values
    camParam.setParameter(CameraParameters::IMAGE_SIZE, CameraParameters::ID_IMAGE_SIZE_3264x2448);
    camParam.setParameter(CameraParameters::IMAGE_FORMAT, CameraParameters::ID_IMAGE_FORMAT_JPEG);
    camParam.setParameter(CameraParameters::PIXEL_FORMAT,
                          CameraParameters::ID_PIXEL_FORMAT_YUV420P);
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
    // TODO::Add handler for this function
    return ret;
}

std::string CameraDeviceV4l2::getDeviceId()
{
    return mDeviceId;
}

int CameraDeviceV4l2::getInfo(struct CameraInfo &camInfo)
{
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
        v4l2_set_control(fd, queryctrl.id, queryctrl.default_value);

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
    v4l2_close(fd);
    return ret;
}

int CameraDeviceV4l2::setBrightness(uint32_t value)
{
    return set_control(V4L2_CID_BRIGHTNESS, value);
}

int CameraDeviceV4l2::setContrast(uint32_t value)
{
    return set_control(V4L2_CID_CONTRAST, value);
}

int CameraDeviceV4l2::setSaturation(uint32_t value)
{
    return set_control(V4L2_CID_SATURATION, value);
}

int CameraDeviceV4l2::setWhiteBalanceMode(uint32_t value)
{
    // TODO :: Translate to v4l2 enum value from exported values
    return set_control(V4L2_CID_AUTO_WHITE_BALANCE, value);
}

int CameraDeviceV4l2::setGamma(uint32_t value)
{
    return set_control(V4L2_CID_GAMMA, value);
}

int CameraDeviceV4l2::setGain(uint32_t value)
{
    return set_control(V4L2_CID_GAIN, value);
}

int CameraDeviceV4l2::setPowerLineFrequency(uint32_t value)
{
    return set_control(V4L2_CID_POWER_LINE_FREQUENCY, value);
}

int CameraDeviceV4l2::setWhiteBalanceTemperature(uint32_t value)
{
    return set_control(V4L2_CID_WHITE_BALANCE_TEMPERATURE, value);
}

int CameraDeviceV4l2::setSharpness(uint32_t value)
{
    return set_control(V4L2_CID_SHARPNESS, value);
}

int CameraDeviceV4l2::setBacklightCompensation(uint32_t value)
{
    return set_control(V4L2_CID_BACKLIGHT_COMPENSATION, value);
}

int CameraDeviceV4l2::setExposureMode(uint32_t value)
{
    return set_control(V4L2_CID_EXPOSURE_AUTO, value);
}

int CameraDeviceV4l2::setExposureAbsolute(uint32_t value)
{
    return set_control(V4L2_CID_EXPOSURE_ABSOLUTE, value);
}

int CameraDeviceV4l2::setSceneMode(uint32_t value)
{
    return 0;
}

int CameraDeviceV4l2::setHue(int32_t value)
{
    return set_control(V4L2_CID_HUE, value);
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
