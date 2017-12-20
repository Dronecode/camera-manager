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
#include <gst/gst.h>
#include <sstream>
#include <unistd.h>
#include <vector>

#include "CameraParameters.h"
#include "ImageCaptureGst.h"

#include "log.h"

ImageCaptureGst::ImageCaptureGst(std::shared_ptr<CameraDevice> camDev)
    : mCamDev(camDev)
    , mState(STATE_IDLE)
    , mWidth(640)
    , mHeight(480)
    , mImgFormat(CameraParameters::ID_IMAGE_FORMAT_JPEG)
    , mPixelFormat(0)
    , mImgPath("/tmp/")
    , mResultCB(nullptr)
{
    mCamDev->getSize(mWidth, mHeight);

    mCamDev->getPixelFormat(mPixelFormat);
}

ImageCaptureGst::~ImageCaptureGst()
{
    stop();
}

int ImageCaptureGst::start(int interval, int count, std::function<void(int result, int seq_num)> cb)
{
    log_debug("%s interval:%d count:%d", __func__, interval, count);
    // Invalid Arguments
    // Either the capture is count based or interval based or count with interval
    if (count <= 0 && interval <= 0) {
        log_error("Invalid Parameters");
        return 1;
    }

    // check & set state
    if (mState != STATE_IDLE)
        return 1;

    mResultCB = cb;
    setState(STATE_IN_PROGRESS);
    // create a thread to capture images
    mThread = std::thread(&ImageCaptureGst::captureThread, this, count, interval);

    return 0;
}

int ImageCaptureGst::stop()
{
    if (mState == STATE_IDLE)
        return 0;

    setState(STATE_IDLE);

    // TODO::thread_join if mThread legal
    return 0;
}

int ImageCaptureGst::setState(int state)
{
    mState = state;

    return 0;
}

int ImageCaptureGst::getState()
{
    return mState;
}

int ImageCaptureGst::setResolution(int imgWidth, int imgHeight)
{
    mWidth = imgWidth;
    mHeight = imgHeight;

    return 0;
}

int ImageCaptureGst::setFormat(int imgFormat)
{
    mImgFormat = imgFormat;

    return 0;
}

int ImageCaptureGst::setLocation(const std::string imgPath)
{
    // TODO::Check if the path is writeable/valid
    log_debug("%s:%s", __func__, imgPath.c_str());
    mImgPath = imgPath;

    return 0;
}

void ImageCaptureGst::captureThread(int num, int interval)
{
    log_debug("captureThread num:%d int:%d", num, interval);
    int ret = -1;
    int count = num;
    int seq_num = 0;
    while (mState == STATE_IN_PROGRESS) {
        log_debug("Current Count : %d", count);
        ret = click(seq_num++);
        if (mResultCB)
            mResultCB(ret, seq_num);

        // Check if the capture is periodic or count based
        if (count <= 0) {
            if (interval > 0)
                sleep(interval);
            else
                break;
        } else {
            count--;
            if (count == 0)
                setState(STATE_IDLE);
            else {
                if (interval > 0)
                    sleep(interval);
            }
        }
    }
}

int ImageCaptureGst::click(int seq_num)
{
    int ret = 0;
    log_debug("%s", __func__);
    ret = createV4l2Pipeline(seq_num);
    return ret;
}

std::string ImageCaptureGst::getGstImgEncName(int format)
{
    switch (format) {
    case CameraParameters::ID_IMAGE_FORMAT_JPEG:
        return "jpegenc";
        break;
    case CameraParameters::ID_IMAGE_FORMAT_RAW:
    case CameraParameters::ID_IMAGE_FORMAT_PNG:
    default:
        return {};
    }
}

std::string ImageCaptureGst::getImgExt(int format)
{
    switch (format) {
    case CameraParameters::ID_IMAGE_FORMAT_JPEG:
        return "jpg";
        break;
    case CameraParameters::ID_IMAGE_FORMAT_RAW:
    case CameraParameters::ID_IMAGE_FORMAT_PNG:
    default:
        return {};
    }
}

std::string ImageCaptureGst::getGstPipelineNameV4l2(int seq_num)
{
    std::string device = mCamDev->getDeviceId();
    if (device.empty())
        return {};

    std::string enc = getGstImgEncName(mImgFormat);
    if (enc.empty())
        return {};

    std::string ext = getImgExt(mImgFormat);
    if (ext.empty())
        return {};

    std::stringstream ss;
    ss << "v4l2src device=" << device + " num-buffers=1"
       << " ! " << enc << " ! "
       << "filesink location=" << mImgPath + "img_" << std::to_string(seq_num) << "." + ext;
    log_debug("Gstreamer pipeline: %s", ss.str().c_str());
    return ss.str();
}

int ImageCaptureGst::createV4l2Pipeline(int seq_num)
{
    int ret = 0;
    GError *error = nullptr;
    GstElement *pipeline;
    GstMessage *msg;
    GstBus *bus;
    std::string pipeline_str = getGstPipelineNameV4l2(seq_num);
    if (pipeline_str.empty()) {
        log_error("Pipeline String error");
        return 1;
    }
    log_debug("pipeline = %s", pipeline_str.c_str());
    pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (!pipeline) {
        log_error("Error creating pipeline");
        if (error)
            g_clear_error(&error);
        return 1;
    }
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_poll(bus, (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR), -1);
    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS: {
        log_debug("EOS\n");
        ret = 0;
        break;
    }
    case GST_MESSAGE_ERROR: {
        GError *err = NULL; /* error to show to users                 */
        gchar *dbg = NULL;  /* additional debug string for developers */

        gst_message_parse_error(msg, &err, &dbg);
        if (err) {
            log_error("ERROR: %s", err->message);
            g_error_free(err);
        }
        if (dbg) {
            log_error("[Debug details: %s]", dbg);
            g_free(dbg);
        }
        ret = 1;
        break;
    }
    default:
        log_error("Unexpected message of type %d", GST_MESSAGE_TYPE(msg));
        ret = 1;
        break;
    }

    gst_message_unref(msg);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    gst_object_unref(bus);

    log_debug("Image Captured Successfully");
    return ret;
}
