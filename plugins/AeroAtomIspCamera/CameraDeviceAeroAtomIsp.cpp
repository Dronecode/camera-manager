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
#include <linux/videodev2.h>
#include <malloc.h>
#include <sys/time.h>
#include <unistd.h>

#include "CameraDeviceAeroAtomIsp.h"
#include "log.h"
#include "v4l2_interface.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define AERO_DEFAULT_WIDTH 640
#define AERO_DEFAULT_HEIGHT 480
#define AERO_DEFAULT_FRAME_RATE 30
#define AERO_DEFAULT_BUFFER_COUNT 4

CameraDeviceAeroAtomIsp::CameraDeviceAeroAtomIsp(std::string device)
    : mDeviceId(device)
    , mState(State::STATE_IDLE)
    , mWidth(AERO_DEFAULT_WIDTH)
    , mHeight(AERO_DEFAULT_HEIGHT)
    , mPixelFormat(CameraParameters::PixelFormat::PIXEL_FORMAT_UYVY)
    , mMode(CameraParameters::Mode::MODE_VIDEO)
    , mFrmRate(AERO_DEFAULT_FRAME_RATE)
    , mCamDefUri{}
    , mFd(-1)
    , mFrameBuffer(nullptr)
    , mFrameBufferSize(0)
    , mFrameBufferCnt(AERO_DEFAULT_BUFFER_COUNT)
{
    log_info("%s path:%s", __func__, mDeviceId.c_str());
}

CameraDeviceAeroAtomIsp::~CameraDeviceAeroAtomIsp()
{
    stop();
    uninit();
}

std::string CameraDeviceAeroAtomIsp::getDeviceId() const
{
    return mDeviceId;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::getInfo(CameraInfo &camInfo) const
{

    /*
     * Fill the camera information.
     * The information can be queried from camera driver/library etc or hardcoded as known from
     * datasheet etc.
     */

    strcpy((char *)camInfo.vendorName, "Intel");
    strcpy((char *)camInfo.modelName, "Aero-Atom-ISP");
    camInfo.firmware_version = 1;
    camInfo.focal_length = 0;
    camInfo.sensor_size_h = AERO_DEFAULT_WIDTH;
    camInfo.sensor_size_v = AERO_DEFAULT_HEIGHT;
    camInfo.resolution_h = AERO_DEFAULT_WIDTH;
    camInfo.resolution_v = AERO_DEFAULT_HEIGHT;
    camInfo.lens_id = 1;
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

bool CameraDeviceAeroAtomIsp::isGstV4l2Src() const
{
    /*
     * Return true if the camera is v4l2 compliant.
     * The gstreamer v4l2src element must be able to open and read from the camera device.
     */

    return false;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::init(CameraParameters &camParam)
{

    /*
     * 1. Initialize the camera device
     * 2. Initialize the CameraParameters object with parameters supported by the camera.
     */

    std::lock_guard<std::mutex> locker(mLock);
    Status retStatus = Status::ERROR_UNKNOWN;

    if (getState() != State::STATE_IDLE)
        return Status::INVALID_STATE;

    retStatus = init();
    if (retStatus != Status::SUCCESS)
        return retStatus;

    setState(State::STATE_INIT);
    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::init()
{

    /* Initialize the camera device */

    int ret = -1;
    Status retStatus = Status::ERROR_UNKNOWN;

    /* Open Camera device */
    mFd = v4l2_open("video2");
    if (mFd < 0) {
        log_error("Error in opening camera device");
        return retStatus;
    }

    /* Set Device ID */
    ret = v4l2_set_input(mFd, 1);
    if (ret)
        goto error;

    /* Query Capabilities */
    struct v4l2_capability vCap;
    ret = v4l2_query_cap(mFd, vCap);
    if (ret)
        goto error;

    /* Set Parameter - preview mode */
    ret = v4l2_set_capturemode(mFd, 0x8000);
    if (ret)
        goto error;

    /* Set Image Format */
    ret = v4l2_set_pixformat(mFd, mWidth, mHeight, V4L2_PIX_FMT_UYVY);
    if (ret)
        goto error;

    /* Allocate User buffer */
    ret = allocFrameBuffer(mFrameBufferCnt, mWidth * mHeight * 2);
    if (ret)
        goto error;

    return Status::SUCCESS;
error:
    v4l2_close(mFd);
    return retStatus;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::uninit()
{
    /*
     * Undo whatever was done in init() call.
     */

    std::lock_guard<std::mutex> locker(mLock);

    if (getState() != State::STATE_IDLE)
        return Status::INVALID_STATE;

    /* Close the Camera Device */
    v4l2_close(mFd);
    mFd = -1;

    /* Free User buffer */
    freeFrameBuffer();

    setState(State::STATE_INIT);
    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::start()
{
    /*
     * Start the camera device to capture images.
     */

    std::lock_guard<std::mutex> locker(mLock);

    if (getState() != State::STATE_INIT)
        return Status::INVALID_STATE;

    int ret = 0;

    /* request buffer */
    ret = v4l2_buf_req(mFd, 4);
    if (ret)
        return Status::ERROR_UNKNOWN;

    struct v4l2_buffer buf;
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_USERPTR;
    for (uint8_t i = 0; i < 4; i++) {
        buf.index = i;
        buf.m.userptr = (unsigned long)mFrameBuffer[i];
        buf.length = mFrameBufferSize;
        ret = v4l2_buf_q(mFd, &buf);
        if (ret)
            return Status::ERROR_UNKNOWN;
    }

    ret = v4l2_streamon(mFd);
    if (ret)
        return Status::ERROR_UNKNOWN;

    setState(State::STATE_RUN);
    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::stop()
{

    std::lock_guard<std::mutex> locker(mLock);

    if (getState() != State::STATE_RUN)
        return Status::INVALID_STATE;

    /*
     * Undo whatever was done in start() call.
     */

    v4l2_streamoff(mFd);

    setState(State::STATE_INIT);
    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::read(CameraData &data)
{
    std::lock_guard<std::mutex> locker(mLock);

    if (getState() != State::STATE_RUN)
        return Status::INVALID_STATE;

    /*
     * Fill the CameraData with frame and its meta-data.
     */

    if (pollCamera(mFd))
        return Status::ERROR_UNKNOWN;

    int ret = 0;
    struct v4l2_buffer buf;
    CLEAR(buf);

    /* dequeue buffer */
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_USERPTR;
    ret = v4l2_buf_dq(mFd, &buf);
    if (ret) {
        log_error("Error in dq buffer");
        return Status::ERROR_UNKNOWN;
    }

    if (!buf.m.userptr) {
        log_error("Null buffer returned");
        return Status::ERROR_UNKNOWN;
    }

    /* TODO:: Check if buffer valid */
    /* TODO:: Check if there is need to change format or size */
    /* TODO:: Use v4l2 buffer timestamp instead for more accuracy */

    struct timeval timeofday;
    gettimeofday(&timeofday, NULL);

    data.sec = timeofday.tv_sec;
    data.nsec = timeofday.tv_usec * 1000;
    data.width = mWidth;
    data.height = mHeight;
    data.stride = mWidth * 2; /* TODO :: Dependent on pixformat */
    data.buf = (void *)buf.m.userptr;
    data.bufSize = buf.bytesused;

    /* queue buffer for refill */
    ret = v4l2_buf_q(mFd, &buf);
    if (ret) {
        log_error("Error in enq buffer");
        return Status::ERROR_UNKNOWN;
    }

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::setParam(CameraParameters &camParam,
                                                       const std::string param,
                                                       const char *param_value,
                                                       const size_t value_size,
                                                       const int param_type)
{
    /*
     * 1. Set the parameter of the camera hardware.s
     * 2. Update the database CameraParameters
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::resetParams(CameraParameters &camParam)
{
    /*
     * 1. Get the default parameter values from driver etc.
     * 2. Set the parameter of the camera hardware with the value
     * 3. Update the database CameraParameters
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::setSize(const uint32_t width, const uint32_t height)
{
    /*
     * 1. Set the width and height of the camera images.
     * 2. Apply the new set value based on the state of camera
     * 3. Unlikely, but some cameras may support setting of resolution on the fly
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::getSize(uint32_t &width, uint32_t &height) const
{
    /*
     * 1. Get the width and height of the camera images.
     */

    width = mWidth;
    height = mHeight;

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::getSupportedSizes(std::vector<Size> &sizes) const
{
    /*
     * 1. Get the list of supported width,height of the camera images.
     * 2. This can be queried from camera driver/library.
     * 3. At times, the supported resolution is dependent on the pixel formats
     */

    return Status::SUCCESS;
}

CameraDevice::Status
CameraDeviceAeroAtomIsp::setPixelFormat(const CameraParameters::PixelFormat format)
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
CameraDeviceAeroAtomIsp::getPixelFormat(CameraParameters::PixelFormat &format) const
{
    /*
     * 1. Get the current pixel format of the camera images.
     */

    format = mPixelFormat;

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::getSupportedPixelFormats(
    std::vector<CameraParameters::PixelFormat> &formats) const
{
    /*
     * 1. Get the list of supported pixel formats of the camera images.
     * 2. This can be queried from camera driver/library.
     * 3. At times, the pixel format and resolution is interdependent
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::setMode(const CameraParameters::Mode mode)
{
    /*
     * 1. Set the mode of the camera device.
     * 2. This may not be relevant to cameras runs on single mode
     */

    mMode = mode;
    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::getMode(CameraParameters::Mode &mode) const
{
    /*
     * 1. Get the current mode of the camera device.
     */

    mode = mMode;
    return Status::SUCCESS;
}

CameraDevice::Status
CameraDeviceAeroAtomIsp::getSupportedModes(std::vector<CameraParameters::Mode> &modes) const
{
    /*
     * 1. Get the supported modes of the camera device.
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::setFrameRate(const uint32_t fps)
{
    /*
     * 1. Set the frame rate of the camera device.
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::getFrameRate(uint32_t &fps) const
{
    /*
     * 1. Get the frame rate of the camera device.
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::getSupportedFrameRates(uint32_t &minFps,
                                                                     uint32_t &maxFps) const
{
    /*
     * 1. Get the supported frame rates of the camera device.
     */

    return Status::SUCCESS;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::setCameraDefinitionUri(const std::string uri)
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

std::string CameraDeviceAeroAtomIsp::getCameraDefinitionUri() const
{
    /*
     * 1. Return the Camera definition file URI.
     */

    return mCamDefUri;
}

CameraDevice::Status CameraDeviceAeroAtomIsp::setState(const CameraDevice::State state)
{
    Status ret = Status::SUCCESS;

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
        /* Free up resources, restart? */
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

CameraDevice::State CameraDeviceAeroAtomIsp::getState() const
{
    return mState;
}

int CameraDeviceAeroAtomIsp::allocFrameBuffer(int bufCnt, size_t bufSize)
{
    log_debug("%s count:%d", __func__, bufCnt);

    int ret = -1;

    /* Check for valid input */
    if (!bufCnt || !bufSize)
        return ret;

    int pageSize = getpagesize();
    size_t bufLen = (bufSize + pageSize - 1) & ~(pageSize - 1);
    log_debug("pagesize=%i buffer_len=%li", pageSize, bufLen);

    mFrameBuffer = (void **)calloc(bufCnt, sizeof(void *));
    for (int i = 0; i < bufCnt; i++) {
        mFrameBuffer[i] = memalign(pageSize, bufLen);
        if (!mFrameBuffer[i]) {
            log_error("memalign failure");
            while (i) {
                i--;
                free(mFrameBuffer[i]);
                mFrameBuffer[i] = NULL;
            }
            break;
        }

        ret = 0;
    }

    mFrameBufferSize = bufLen;
    log_debug("%s Exit", __func__);
    return ret;
}

int CameraDeviceAeroAtomIsp::freeFrameBuffer()
{
    log_debug("%s", __func__);

    if (!mFrameBuffer)
        return 0;

    for (uint8_t i = 0; i < mFrameBufferCnt; i++) {
        free(mFrameBuffer[i]);
    }
    free(mFrameBuffer);
    mFrameBuffer = NULL;

    return 0;
}

int CameraDeviceAeroAtomIsp::pollCamera(int fd)
{
    int ret = -1;
    while (mState == State::STATE_RUN) {
        fd_set fds;
        struct timeval tv;
        int r;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == r) {
            if (EINTR == errno)
                continue;
            log_error("select error");
            ret = -1;
            break;
        }

        if (0 == r && mState == State::STATE_RUN) {
            log_error("select timeout");
            stop();
            uninit();
            init();
            start();
            continue;
        }

        ret = 0;
        break;
    }

    return ret;
}
