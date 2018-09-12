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
#include <sys/time.h>

#include "CameraDeviceRealSense.h"

#define RS_DEFAULT_WIDTH 640
#define RS_DEFAULT_HEIGHT 480
#define RS_DEFAULT_FRAME_RATE 60

int CameraDeviceRealSense::sStrmCnt = 0;

static void rainbow_scale(double value, uint8_t rgb[])
{
    rgb[0] = rgb[1] = rgb[2] = 0;

    if (value <= 0.0)
        return;

    if (value < 0.25) { // RED to YELLOW
        rgb[0] = 255;
        rgb[1] = (uint8_t)255 * (value / 0.25);
    } else if (value < 0.5) { // YELLOW to GREEN
        rgb[0] = (uint8_t)255 * (1 - ((value - 0.25) / 0.25));
        rgb[1] = 255;
    } else if (value < 0.75) { // GREEN to CYAN
        rgb[1] = 255;
        rgb[2] = (uint8_t)255 * (value - 0.5 / 0.25);
    } else if (value < 1.0) { // CYAN to BLUE
        rgb[1] = (uint8_t)255 * (1 - ((value - 0.75) / 0.25));
        rgb[2] = 255;
    } else { // BLUE
        rgb[2] = 255;
    }
}

CameraDeviceRealSense::CameraDeviceRealSense(std::string device)
    : mDeviceId(device)
    , mState(State::STATE_IDLE)
    , mWidth(RS_DEFAULT_WIDTH)
    , mHeight(RS_DEFAULT_HEIGHT)
    , mPixelFormat(CameraParameters::PixelFormat::PIXEL_FORMAT_RGB24)
    , mMode(CameraParameters::Mode::MODE_VIDEO)
    , mFrmRate(RS_DEFAULT_FRAME_RATE)
    , mCamDefUri{}
    , mFrameBuffer(nullptr)
    , mFrameBufferSize(0)
    , mRSDev(nullptr)
    , mRSCtx(nullptr)
    , mRSStream(-1)
{
    log_info("%s path:%s", __func__, mDeviceId.c_str());
    if (device == "rsdepth")
        mRSStream = RS_STREAM_DEPTH;
    else if (device == "rsir")
        mRSStream = RS_STREAM_INFRARED;
    else if (device == "rsir2")
        mRSStream = RS_STREAM_INFRARED2;
    else
        log_error("Unknown RS Stream");
}

CameraDeviceRealSense::~CameraDeviceRealSense()
{
    stop();
    uninit();
}

std::string CameraDeviceRealSense::getDeviceId() const
{
    return mDeviceId;
}

CameraDevice::Status CameraDeviceRealSense::getInfo(CameraInfo &camInfo) const
{

    /*
     * Fill the camera information.
     * The information can be queried from camera driver/library etc or hardcoded as known from
     * datasheet etc.
     */

    strcpy((char *)camInfo.vendorName, "Intel");
    strcpy((char *)camInfo.modelName, "RS200");
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

bool CameraDeviceRealSense::isGstV4l2Src() const
{
    /*
     * Return true if the camera is v4l2 compliant.
     * The gstreamer v4l2src element must be able to open and read from the camera device.
     */

    return false;
}

CameraDevice::Status CameraDeviceRealSense::init(CameraParameters &camParam)
{

    /*
     * 1. Initialize the camera device
     * 2. Initialize the CameraParameters object with parameters supported by the camera.
     */
    std::lock_guard<std::mutex> locker(mLock);

    setState(State::STATE_INIT);
    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceRealSense::uninit()
{
    /*
     * Undo whatever was done in init() call.
     */

    std::lock_guard<std::mutex> locker(mLock);

    setState(State::STATE_INIT);
    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceRealSense::start()
{
    std::lock_guard<std::mutex> locker(mLock);

    if (getState() != State::STATE_INIT)
        return Status::INVALID_STATE;

    /*
     * Start the camera device to capture images.
     */

    /* librealsense */
    rs_error *e = 0;
    mRSCtx = rs_create_context(RS_API_VERSION, &e);
    if (e) {
        log_error("rs_error was raised when calling %s(%s): %s", rs_get_failed_function(e),
                  rs_get_failed_args(e), rs_get_error_message(e));
        log_error("Current librealsense api version %d", rs_get_api_version(NULL));
        log_error("Compiled for librealsense api version %d", RS_API_VERSION);
        return Status::ERROR_UNKNOWN;
    }

    mRSDev = rs_get_device(mRSCtx, 0, NULL);
    if (!mRSDev) {
        log_error("Unable to access realsense device");
        rs_delete_context(mRSCtx, NULL);
        return Status::ERROR_UNKNOWN;
    }

    /* Configure all streams to run at VGA resolution at 60 frames per second */
    if (mRSStream == RS_STREAM_INFRARED || mRSStream == RS_STREAM_INFRARED2) {
        rs_enable_stream(mRSDev, (rs_stream)mRSStream, mWidth, mHeight, RS_FORMAT_Y8, mFrmRate, &e);
    } else {
        rs_enable_stream(mRSDev, RS_STREAM_DEPTH, mWidth, mHeight, RS_FORMAT_Z16, mFrmRate, NULL);
    }

    sStrmCnt++;

    if (sStrmCnt == 3) {
        /* start once all rs streams are enabled*/
        rs_start_device(mRSDev, NULL);

        if (rs_device_supports_option(mRSDev, RS_OPTION_R200_EMITTER_ENABLED, NULL))
            rs_set_device_option(mRSDev, RS_OPTION_R200_EMITTER_ENABLED, 1, NULL);
        if (rs_device_supports_option(mRSDev, RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED, NULL))
            rs_set_device_option(mRSDev, RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED, 1, NULL);
    }

    mFrameBufferSize = mWidth * mHeight * 3;
    mFrameBuffer = new uint8_t[mFrameBufferSize];
    if (!mFrameBuffer) {
        log_error("Memory alloc for frame buf failed");
        mFrameBufferSize = 0;
        rs_delete_context(mRSCtx, NULL);
        return Status::NO_MEMORY;
    }

    setState(State::STATE_RUN);
    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceRealSense::stop()
{
    std::lock_guard<std::mutex> locker(mLock);

    if (getState() != State::STATE_RUN)
        return Status::INVALID_STATE;

    /*
     * Undo whatever was done in start() call.
     */

    rs_delete_context(mRSCtx, NULL);
    delete[] mFrameBuffer;
    mFrameBufferSize = 0;

    setState(State::STATE_INIT);
    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceRealSense::read(CameraData &data)
{
    std::lock_guard<std::mutex> locker(mLock);

    if (getState() != State::STATE_RUN)
        return Status::INVALID_STATE;

    /*
     * Fill the CameraData with frame and its meta-data.
     */

    rs_wait_for_frames(mRSDev, NULL);

    rs_error *e = 0;
    if (mRSStream == RS_STREAM_INFRARED || mRSStream == RS_STREAM_INFRARED2) {
        uint8_t *ir = (uint8_t *)rs_get_frame_data(mRSDev, (rs_stream)mRSStream, &e);
        if (!ir) {
            log_error("rs_error was raised when calling %s(%s): %s", rs_get_failed_function(e),
                      rs_get_failed_args(e), rs_get_error_message(e));
            log_error("No infrared data. Not building frame");

            return Status::ERROR_UNKNOWN;
        }

        for (int i = 0, end = mWidth * mHeight; i < end; ++i) {
            mFrameBuffer[3 * i] = ir[i];
            mFrameBuffer[3 * i + 1] = ir[i];
            mFrameBuffer[3 * i + 2] = ir[i];
        }
    } else {
        uint16_t *depth = (uint16_t *)rs_get_frame_data(mRSDev, RS_STREAM_DEPTH, NULL);
        if (!depth) {
            log_error("No depth data. Not building frame");
            return Status::ERROR_UNKNOWN;
        }

        for (int i = 0, end = mWidth * mHeight; i < end; ++i) {
            uint8_t rainbow[3];
            rainbow_scale((double)depth[i], rainbow);

            mFrameBuffer[3 * i] = rainbow[0];
            mFrameBuffer[3 * i + 1] = rainbow[1];
            mFrameBuffer[3 * i + 2] = rainbow[2];
        }
    }

    struct timeval timeofday;
    gettimeofday(&timeofday, NULL);

    data.sec = timeofday.tv_sec;
    data.nsec = timeofday.tv_usec * 1000;
    data.width = mWidth;
    data.height = mHeight;
    data.stride = mWidth * 3; // TODO :: Dependent on pixformat
    data.buf = mFrameBuffer;
    data.bufSize = mFrameBufferSize;

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceRealSense::setParam(CameraParameters &camParam,
                                                     const std::string param,
                                                     const char *param_value,
                                                     const size_t value_size, const int param_type)
{
    /*
     * 1. Set the parameter of the camera hardware.
     * 2. Update the database CameraParameters
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceRealSense::resetParams(CameraParameters &camParam)
{
    /*
     * 1. Get the default parameter values from driver etc.
     * 2. Set the parameter of the camera hardware with the value
     * 3. Update the database CameraParameters
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceRealSense::setSize(const uint32_t width, const uint32_t height)
{
    /*
     * 1. Set the width and height of the camera images.
     * 2. Apply the new set value based on the state of camera
     * 3. Unlikely, but some cameras may support setting of resolution on the fly
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceRealSense::getSize(uint32_t &width, uint32_t &height) const
{
    /*
     * 1. Get the width and height of the camera images.
     */

    width = mWidth;
    height = mHeight;

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceRealSense::getSupportedSizes(std::vector<Size> &sizes) const
{
    /*
     * 1. Get the list of supported width,height of the camera images.
     * 2. This can be queried from camera driver/library.
     * 3. At times, the supported resolution is dependent on the pixel formats
     */

    return Status::SUCCESS;
}

CameraDevice::Status
CameraDeviceRealSense::setPixelFormat(const CameraParameters::PixelFormat format)
{
    /*
     * 1. Set the pixel format of the camera images.
     * 2. Apply the new set value based on the state of camera
     * 3. Unlikely, but some cameras may support setting of pixel format on the fly
     * 4. Pixel format conversion logic can be added to support more formats
     */

    return Status::SUCCESS;
}

CameraDevice::Status
CameraDeviceRealSense::getPixelFormat(CameraParameters::PixelFormat &format) const
{
    /*
     * 1. Get the current pixel format of the camera images.
     */

    format = mPixelFormat;

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceRealSense::getSupportedPixelFormats(
    std::vector<CameraParameters::PixelFormat> &formats) const
{
    /*
     * 1. Get the list of supported pixel formats of the camera images.
     * 2. This can be queried from camera driver/library.
     * 3. At times, the pixel format and resolution is interdependent
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceRealSense::setMode(const CameraParameters::Mode mode)
{
    /*
     * 1. Set the mode of the camera device.
     * 2. This may not be relevant to cameras runs on single mode
     */

    mMode = mode;
    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceRealSense::getMode(CameraParameters::Mode &mode) const
{
    /*
     * 1. Get the current mode of the camera device.
     */

    mode = mMode;
    return Status::SUCCESS;
}

CameraDevice::Status
CameraDeviceRealSense::getSupportedModes(std::vector<CameraParameters::Mode> &modes) const
{
    /*
     * 1. Get the supported modes of the camera device.
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceRealSense::setFrameRate(const uint32_t fps)
{
    /*
     * 1. Set the frame rate of the camera device.
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceRealSense::getFrameRate(uint32_t &fps) const
{
    /*
     * 1. Get the frame rate of the camera device.
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceRealSense::getSupportedFrameRates(uint32_t &minFps,
                                                                   uint32_t &maxFps) const
{
    /*
     * 1. Get the supported frame rates of the camera device.
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceRealSense::setCameraDefinitionUri(const std::string uri)
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

std::string CameraDeviceRealSense::getCameraDefinitionUri() const
{
    /*
     * 1. Return the Camera definition file URI.
     */

    return mCamDefUri;
}

CameraDevice::Status CameraDeviceRealSense::setState(const CameraDevice::State state)
{
    Status ret = Status::SUCCESS;
    // log_debug("%s : %d", __func__, state);

    if (mState == state)
        return ret;

    if (state == State::STATE_ERROR) {
        mState = state;
        return ret;
    }

    /* IDLE -> INIT -> RUN -> INIT -> IDLE*/
    switch (mState) {
    case State::STATE_IDLE:
        if (state == State::STATE_INIT)
            mState = state;
        break;
    case State::STATE_INIT:
        if (state == State::STATE_IDLE || state == State::STATE_RUN)
            mState = state;
        break;
    case State::STATE_RUN:
        if (state == State::STATE_INIT)
            mState = state;
        break;
    case State::STATE_ERROR:
        log_info("In Error State");
        // Free up resources, restart?
        if (state == State::STATE_IDLE)
            mState = state;
        break;
    default:
        break;
    }

    if (mState != state) {
        ret = Status::INVALID_STATE;
        log_error("InValid State Transition");
    }

    return ret;
}

CameraDevice::State CameraDeviceRealSense::getState() const
{
    return mState;
}
