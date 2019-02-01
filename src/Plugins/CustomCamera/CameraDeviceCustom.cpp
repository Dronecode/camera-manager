/*
 * This file is part of the Dronecode Camera Manager
 *
 * Copyright (C) 2018  Intel Corporation. All rights reserved.
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
#include <iostream>

#include "CameraDeviceCustom.h"

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 360
#define DEFAULT_FRAME_RATE 30

const char CameraDeviceCustom::PARAMETER_CUSTOM_UINT8[] = "custom-uint8";
const int CameraDeviceCustom::ID_PARAMETER_CUSTOM_UINT8 = 101;
const char CameraDeviceCustom::PARAMETER_CUSTOM_UINT32[] = "custom-uint32";
const int CameraDeviceCustom::ID_PARAMETER_CUSTOM_UINT32 = 102;
const char CameraDeviceCustom::PARAMETER_CUSTOM_INT32[] = "custom-int32";
const int CameraDeviceCustom::ID_PARAMETER_CUSTOM_INT32 = 104;
const char CameraDeviceCustom::PARAMETER_CUSTOM_REAL32[] = "custom-real32";
const int CameraDeviceCustom::ID_PARAMETER_CUSTOM_REAL32 = 105;
const char CameraDeviceCustom::PARAMETER_CUSTOM_ENUM[] = "custom-enum";
const int CameraDeviceCustom::ID_PARAMETER_CUSTOM_ENUM = 106;

CameraDeviceCustom::CameraDeviceCustom(std::string device)
    : mDeviceId(device)
    , mState(State::STATE_IDLE)
    , mWidth(DEFAULT_WIDTH)
    , mHeight(DEFAULT_HEIGHT)
    , mPixelFormat(CameraParameters::PixelFormat::PIXEL_FORMAT_RGB24)
    , mMode(CameraParameters::Mode::MODE_VIDEO)
    , mFrmRate(DEFAULT_FRAME_RATE)
    , mCamDefUri{}
    , mOvText(device)
{
    log_info("%s path:%s", __func__, mDeviceId.c_str());
}

CameraDeviceCustom::~CameraDeviceCustom()
{
    stop();
    uninit();
}

std::string CameraDeviceCustom::getDeviceId() const

{
    return mDeviceId;
}

CameraDevice::Status CameraDeviceCustom::getInfo(CameraInfo &camInfo) const
{

    /*
     * Fill the camera information.
     * The information can be queried from camera driver/library etc or hardcoded as known from
     * datasheet etc.
     */

    strcpy((char *)camInfo.vendorName, "Custom");
    strcpy((char *)camInfo.modelName, "Custom-Camera");
    camInfo.firmware_version = 1;
    camInfo.focal_length = 0;
    camInfo.sensor_size_h = 0;
    camInfo.sensor_size_v = 0;
    camInfo.resolution_h = 0;
    camInfo.resolution_v = 0;
    camInfo.lens_id = 0;
    camInfo.flags = ~0u;
    camInfo.cam_definition_version = 1;
    if (!mCamDefUri.empty()) {
        if (sizeof(camInfo.cam_definition_uri) > mCamDefUri.size()) {
            strcpy((char *)camInfo.cam_definition_uri, mCamDefUri.c_str());
        } else {
            log_error("URI length bigger than permitted");
        }
    }
    return CameraDevice::Status::SUCCESS;
}

bool CameraDeviceCustom::isGstV4l2Src() const
{
    /*
     * Return true if the camera is v4l2 compliant.
     * The gstreamer v4l2src element must be able to open and read from the camera device.
     */

    return false;
}

CameraDevice::Status CameraDeviceCustom::init(CameraParameters &camParam)
{

    /*
     * 1. Initialize the camera device
     * 2. Initialize the CameraParameters object with parameters supported by the camera.
     */

    camParam.setParameter(CameraParameters::CAMERA_MODE, (uint32_t)mMode);
    camParam.setParameterIdType(PARAMETER_CUSTOM_UINT8, ID_PARAMETER_CUSTOM_UINT8,
                                CameraParameters::PARAM_TYPE_UINT8);
    camParam.setParameter(PARAMETER_CUSTOM_UINT8, (uint8_t)50);
    camParam.setParameterIdType(PARAMETER_CUSTOM_UINT32, ID_PARAMETER_CUSTOM_UINT32,
                                CameraParameters::PARAM_TYPE_UINT32);
    camParam.setParameter(PARAMETER_CUSTOM_UINT32, (uint32_t)50);
    camParam.setParameterIdType(PARAMETER_CUSTOM_INT32, ID_PARAMETER_CUSTOM_INT32,
                                CameraParameters::PARAM_TYPE_INT32);
    camParam.setParameter(PARAMETER_CUSTOM_INT32, (int32_t)-10);
    camParam.setParameterIdType(PARAMETER_CUSTOM_REAL32, ID_PARAMETER_CUSTOM_REAL32,
                                CameraParameters::PARAM_TYPE_REAL32);
    camParam.setParameter(PARAMETER_CUSTOM_REAL32, (float)0);
    camParam.setParameterIdType(PARAMETER_CUSTOM_ENUM, ID_PARAMETER_CUSTOM_ENUM,
                                CameraParameters::PARAM_TYPE_UINT32);
    camParam.setParameter(PARAMETER_CUSTOM_ENUM, (uint32_t)0);

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceCustom::uninit()
{
    /*
     * Undo whatever was done in init() call.
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceCustom::start()
{
    /*
     * Start the camera device to capture images.
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceCustom::stop()
{
    /*
     * Undo whatever was done in start() call.
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceCustom::read(CameraData &data)
{
    /*
     * Fill the CameraData with frame and its meta-data.
     * This function need not be implemented for v4l2 devices
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceCustom::setParam(CameraParameters &camParam,
                                                  const std::string param, const char *param_value,
                                                  const size_t value_size, const int param_type)
{
    /*
     * 1. Set the parameter of the camera hardware.
     * 2. Update the database CameraParameters
     */

    int ret = 0;
    CameraParameters::cam_param_union_t u;
    memcpy(&u.param_float, param_value, sizeof(float));
    int paramId = camParam.getParameterID(param);
    switch (paramId) {
    case ID_PARAMETER_CUSTOM_UINT8:
        log_info("Parameter: %s Value: %d", param.c_str(), u.param_uint8);
        camParam.setParameter(param, u.param_uint8);
        break;
    case ID_PARAMETER_CUSTOM_UINT32:
        log_info("Parameter: %s Value: %d", param.c_str(), u.param_uint32);
        camParam.setParameter(param, u.param_uint32);
        break;
    case ID_PARAMETER_CUSTOM_INT32:
        log_info("Parameter: %s Value: %d", param.c_str(), u.param_int32);
        camParam.setParameter(param, u.param_int32);
        break;
    case ID_PARAMETER_CUSTOM_REAL32:
        log_info("Parameter: %s Value: %f", param.c_str(), u.param_float);
        camParam.setParameter(param, u.param_float);
        break;
    case ID_PARAMETER_CUSTOM_ENUM:
        log_info("Parameter: %s Value: %d", param.c_str(), u.param_uint32);
        camParam.setParameter(param, u.param_uint32);
        break;
    default:
        ret = -ENOTSUP;
        break;
    }

    if (!ret)
        return Status::SUCCESS;
    else
        return Status::NOT_SUPPORTED;
}

CameraDevice::Status CameraDeviceCustom::resetParams(CameraParameters &camParam)
{
    /*
     * 1. Get the default parameter values from driver etc.
     * 2. Set the parameter of the camera hardware with the value
     * 3. Update the database CameraParameters
     */

    CameraDevice::Status ret = Status::SUCCESS;

    // TODO :: The default params need to be stored in DS during init
    camParam.setParameter(CameraParameters::CAMERA_MODE,
                          (uint32_t)CameraParameters::Mode::MODE_VIDEO);
    camParam.setParameter(PARAMETER_CUSTOM_UINT8, (uint8_t)50);
    camParam.setParameter(PARAMETER_CUSTOM_UINT32, (uint32_t)50);
    camParam.setParameter(PARAMETER_CUSTOM_INT32, (int32_t)-10);
    camParam.setParameter(PARAMETER_CUSTOM_REAL32, (float)0);
    camParam.setParameter(PARAMETER_CUSTOM_ENUM, (uint32_t)0);

    return ret;
}

CameraDevice::Status CameraDeviceCustom::setSize(const uint32_t width, const uint32_t height)
{
    /*
     * 1. Set the width and height of the camera images.
     * 2. Apply the new set value based on the state of camera
     * 3. Unlikely, but some cameras may support setting of resolution on the fly
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceCustom::getSize(uint32_t &width, uint32_t &height) const
{
    /*
     * 1. Get the width and height of the camera images.
     */

    width = mWidth;
    height = mHeight;

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceCustom::getSupportedSizes(std::vector<Size> &sizes) const
{
    /*
     * 1. Get the list of supported width,height of the camera images.
     * 2. This can be queried from camera driver/library.
     * 3. At times, the supported resolution is dependent on the pixel formats
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceCustom::setPixelFormat(const CameraParameters::PixelFormat format)
{
    /*
     * 1. Set the pixel format of the camera images.
     * 2. Apply the new set value based on the state of camera
     * 3. Unlikely, but some cameras may support setting of pixel format on the fly
     * 4. Pixel format conversion logic can be added to support more formats
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceCustom::getPixelFormat(CameraParameters::PixelFormat &format) const
{
    /*
     * 1. Get the current pixel format of the camera images.
     */

    format = mPixelFormat;

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceCustom::getSupportedPixelFormats(
    std::vector<CameraParameters::PixelFormat> &formats) const
{
    /*
     * 1. Get the list of supported pixel formats of the camera images.
     * 2. This can be queried from camera driver/library.
     * 3. At times, the pixel format and resolution is interdependent
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceCustom::setMode(const CameraParameters::Mode mode)
{
    /*
     * 1. Set the mode of the camera device.
     * 2. This may not be relevant to cameras runs on single mode
     */

    mMode = mode;
    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceCustom::getMode(CameraParameters::Mode &mode) const
{
    /*
     * 1. Get the current mode of the camera device.
     */

    mode = mMode;
    return Status::SUCCESS;
}

CameraDevice::Status
CameraDeviceCustom::getSupportedModes(std::vector<CameraParameters::Mode> &modes) const
{
    /*
     * 1. Get the supported modes of the camera device.
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceCustom::setFrameRate(const uint32_t fps)
{
    /*
     * 1. Set the frame rate of the camera device.
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceCustom::getFrameRate(uint32_t &fps) const
{
    /*
     * 1. Get the frame rate of the camera device.
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceCustom::getSupportedFrameRates(uint32_t &minFps, uint32_t &maxFps)
{
    /*
     * 1. Get the supported frame rates of the camera device.
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceCustom::setCameraDefinitionUri(const std::string uri)
{
    /*
     * 1. Set the URI address of the camera definition file for the camera device
     * 2. The URI can be read from a configuration file and set by CameraServer
     * 3. The CameraDevice internally can generate definition XML file based on parameter supported
     *    by the camera hardware and place it appropriately to host the file for download by GCS
     */

    if (uri.empty())
        return Status::INVALID_ARGUMENT;

    mCamDefUri = uri;
    return Status::SUCCESS;
}

std::string CameraDeviceCustom::getCameraDefinitionUri() const
{
    /*
     * 1. Return the Camera definition file URI.
     */

    return mCamDefUri;
}

std::string CameraDeviceCustom::getOverlayText() const
{
    /*
     * 1. Return the text that can be overlayed on the camera frames.
     * 2. This is just to enable CameraDevice to add informational text on the frames.
     * 3. The real overlay will be done by the object that reads and uses camera frames.
     */

    return mOvText;
}
