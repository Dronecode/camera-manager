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

#define DEFAULT_IMAGE_FILE_FORMAT CameraParameters::IMAGE_FILE_JPEG
#define DEFAULT_FILE_PATH "/tmp/"

ImageCaptureGst::ImageCaptureGst(std::shared_ptr<CameraDevice> camDev)
    : mCamDev(camDev)
    , mState(STATE_IDLE)
    , mWidth(0)
    , mHeight(0)
    , mFormat(DEFAULT_IMAGE_FILE_FORMAT)
    , mInterval(0)
    , mPath(DEFAULT_FILE_PATH)
    , mResultCB(nullptr)
{
    log_info("%s Device:%s", __func__, mCamDev->getDeviceId().c_str());

    mCamDev->getSize(mCamWidth, mCamHeight);
    mCamDev->getPixelFormat(mCamPixFormat);
}

ImageCaptureGst::ImageCaptureGst(std::shared_ptr<CameraDevice> camDev,
                                 struct ImageSettings &imgSetting)
    : mCamDev(camDev)
    , mState(STATE_IDLE)
    , mWidth(imgSetting.width)
    , mHeight(imgSetting.height)
    , mFormat(imgSetting.fileFormat)
    , mInterval(0)
    , mPath(DEFAULT_FILE_PATH)
    , mResultCB(nullptr)
{
    log_info("%s Device:%s with settings", __func__, mCamDev->getDeviceId().c_str());

    mCamDev->getSize(mCamWidth, mCamHeight);
    mCamDev->getPixelFormat(mCamPixFormat);
}

ImageCaptureGst::~ImageCaptureGst()
{
    stop();
}

int ImageCaptureGst::init()
{
    log_info("%s::%s", typeid(this).name(), __func__);

    if (getState() != STATE_IDLE) {
        log_error("Invalid State : %d", getState());
        return -1;
    }

    setState(STATE_INIT);
    return 0;
}

int ImageCaptureGst::uninit()
{
    log_info("%s::%s", typeid(this).name(), __func__);

    if (getState() != STATE_INIT && getState() != STATE_ERROR) {
        log_error("Invalid State : %d", getState());
        return -1;
    }

    setState(STATE_IDLE);
    return 0;
}

int ImageCaptureGst::start(int interval, int count, std::function<void(int result, int seq_num)> cb)
{
    int ret = 0;
    log_info("%s::%s interval:%d count:%d", typeid(this).name(), __func__, interval, count);
    // Invalid Arguments
    // Either the capture is count based or interval based or count with interval
    if (count <= 0 && interval <= 0) {
        log_error("Invalid Parameters");
        return 1;
    }

    // check & set state
    if (getState() != STATE_INIT) {
        log_error("Invalid State : %d", getState());
        return -1;
    }

    mResultCB = cb;
    mInterval = interval;
    setState(STATE_RUN);

    if (count == 1) {
        // There will be no stop call
        ret = click(1);
        setState(STATE_INIT);
        if (mResultCB)
            mResultCB(ret, 1);
    } else {
        // create a thread to capture images
        mThread = std::thread(&ImageCaptureGst::captureThread, this, count);
    }

    return 0;
}

int ImageCaptureGst::stop()
{
    log_info("%s::%s", typeid(this).name(), __func__);

    setState(STATE_INIT);

    if (mThread.joinable())
        mThread.join();

    return 0;
}

int ImageCaptureGst::setState(int state)
{
    int ret = 0;
    log_debug("%s : %d", __func__, state);

    if (mState == state)
        return 0;

    if (state == STATE_ERROR) {
        mState = state;
        return 0;
    }

    switch (mState) {
    case STATE_IDLE:
        if (state == STATE_INIT)
            mState = state;
        break;
    case STATE_INIT:
        if (state == STATE_IDLE || state == STATE_RUN)
            mState = state;
        break;
    case STATE_RUN:
        if (state == STATE_INIT)
            mState = state;
        break;
    case STATE_ERROR:
        log_info("In Error State");
        // Free up resources, restart?
        if (state == STATE_IDLE)
            mState = state;
        break;
    default:
        break;
    }

    if (mState != state) {
        ret = -1;
        log_error("InValid State Transition");
    }

    return ret;
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

int ImageCaptureGst::setInterval(int interval)
{
    if (interval < 0)
        return -1;

    mInterval = interval;

    return 0;
}

int ImageCaptureGst::getInterval()
{
    return mInterval;
}

int ImageCaptureGst::setFormat(CameraParameters::IMAGE_FILE_FORMAT imgFormat)
{
    if (imgFormat <= CameraParameters::IMAGE_FILE_MIN
        || imgFormat >= CameraParameters::IMAGE_FILE_MAX) {
        log_error("Invalid Pixel format");
        return 1;
    }

    mFormat = imgFormat;

    return 0;
}

int ImageCaptureGst::setLocation(const std::string imgPath)
{
    // TODO::Check if the path is writeable/valid
    log_debug("%s:%s", __func__, imgPath.c_str());
    mPath = imgPath;

    return 0;
}

void ImageCaptureGst::captureThread(int num)
{
    log_debug("captureThread num:%d int:%d", num, mInterval);
    int ret = -1;
    int count = num;
    int seq_num = 0;
    while (mState == STATE_RUN) {
        ret = click(seq_num++);
        if (getState() != STATE_RUN)
            continue;

        if (mResultCB)
            mResultCB(ret, seq_num);

        if (ret) {
            log_error("Error in Image Capture");
            setState(STATE_ERROR);
            continue;
        }

        // Check if the capture is periodic or count(w/wo interval) based
        if (count <= 0) {
            if (mInterval > 0)
                sleep(mInterval);
            else
                break;
        } else {
            log_debug("Current Count : %d", count);
            count--;
            if (count == 0)
                setState(STATE_INIT);
            else {
                if (mInterval > 0)
                    sleep(mInterval);
            }
        }
    }
}

int ImageCaptureGst::click(int seq_num)
{
    log_debug("%s", __func__);

    int ret = 0;
    if (mCamDev->isGstV4l2Src())
        ret = createV4l2Pipeline(seq_num);
    else
        ret = createAppsrcPipeline(seq_num);
    return ret;
}

std::string ImageCaptureGst::getGstImgEncName(int format)
{
    switch (format) {
    case CameraParameters::IMAGE_FILE_JPEG:
        return "jpegenc";
        break;
    case CameraParameters::IMAGE_FILE_PNG:
    case CameraParameters::IMAGE_FILE_RAW:
    default:
        return {};
    }
}

std::string ImageCaptureGst::getGstPixFormat(int pixFormat)
{
    switch (pixFormat) {
    case CameraParameters::ID_PIXEL_FORMAT_RGB24:
        return "RGB";
        break;
    default:
        return {};
    }
}

std::string ImageCaptureGst::getImgExt(int format)
{
    switch (format) {
    case CameraParameters::IMAGE_FILE_JPEG:
        return "jpg";
    case CameraParameters::IMAGE_FILE_PNG:
        return "png";
    case CameraParameters::IMAGE_FILE_RAW:
    default:
        return "raw";
    }
}

std::string ImageCaptureGst::getGstPipelineNameV4l2(int seq_num)
{
    std::string device = mCamDev->getDeviceId();
    if (device.empty())
        return {};

    std::string enc = getGstImgEncName(mFormat);
    if (enc.empty())
        return {};

    std::string ext = getImgExt(mFormat);
    if (ext.empty())
        return {};

    std::stringstream filter;
    filter << "video/x-raw, ";
    if (mWidth > 0 && mHeight > 0) {
        filter << " width=" << std::to_string(mWidth) << ", height=" << std::to_string(mHeight);
    }

    std::stringstream ss;
    ss << "v4l2src device=" << device + " num-buffers=1"
       << " ! " << filter.str() << " ! " << enc << " ! "
       << "filesink location=" << mPath + "img_" << std::to_string(seq_num) << "." + ext;
    log_debug("Gstreamer pipeline: %s", ss.str().c_str());
    return ss.str();
}

int ImageCaptureGst::createV4l2Pipeline(int seq_num)
{
    log_info("%s", __func__);

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
        log_info("Image Captured Successfully");
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

    return ret;
}

static void cbNeedData(GstElement *appsrc, guint unused_size, ImageCaptureGst *obj)
{
    log_debug("%s", __func__);

    GstFlowReturn ret;
    std::vector<uint8_t> frame = obj->mCamDev->read();
    uint8_t *data = &frame[0];
    gsize size = frame.size(); // width*height*3/2/1.5
    gsize offset = 0;
    gsize maxsize = size;
    GstBuffer *buffer
        = gst_buffer_new_wrapped_full((GstMemoryFlags)0, data, maxsize, offset, size, NULL, NULL);
    assert(buffer);

    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);
    if (ret != GST_FLOW_OK) {
        /* some error */
        log_error("Error in sending data to gst pipeline");
    }
    g_signal_emit_by_name(appsrc, "end-of-stream", &ret);
    // gst_app_src_end_of_stream (appsrc);
}

int ImageCaptureGst::createAppsrcPipeline(int seq_num)
{
    log_info("%s", __func__);

    int ret = 0;
    GstElement *pipeline, *appsrc, *enc, *imagesink;
    GstMessage *msg;

    std::string encname = getGstImgEncName(mFormat);
    if (encname.empty())
        return {};

    std::string fmt = getGstPixFormat(mCamPixFormat);
    if (fmt.empty())
        return {};

    std::string ext = getImgExt(mFormat);
    if (ext.empty())
        return {};

    /* setup pipeline */
    pipeline = gst_pipeline_new("pipeline");
    appsrc = gst_element_factory_make("appsrc", "source");
    enc = gst_element_factory_make(encname.c_str(), "enc");
    imagesink = gst_element_factory_make("filesink", "imagesink");
    std::string filepath = mPath + "img_" + std::to_string(seq_num) + "." + ext;
    g_object_set(G_OBJECT(imagesink), "location", filepath.c_str(), NULL);

    log_info("Pipeline Format:%d, width:%d, Height:%d", mCamPixFormat, mWidth, mHeight);
    log_info("Pipeline File:%s", filepath.c_str());

    /* setup */
    g_object_set(G_OBJECT(appsrc), "caps",
                 gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, fmt.c_str(), "width",
                                     G_TYPE_INT, mWidth, "height", G_TYPE_INT, mHeight, "framerate",
                                     GST_TYPE_FRACTION, 0, 1, NULL),
                 NULL);
    gst_bin_add_many(GST_BIN(pipeline), appsrc, enc, imagesink, NULL);
    gst_element_link_many(appsrc, enc, imagesink, NULL);

    /* setup appsrc */
    g_object_set(G_OBJECT(appsrc), "stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
                 "format", GST_FORMAT_TIME, "is-live", TRUE, NULL);
    g_signal_connect(appsrc, "need-data", G_CALLBACK(cbNeedData), this);

    /* play */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* add watch for messages */
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    // gst_bus_add_watch (bus, (GstBusFunc) bus_message, NULL);
    msg = gst_bus_poll(bus, (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR), -1);
    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS: {
        log_error("EOS\n");
        ret = 0;
        break;
    }
    case GST_MESSAGE_ERROR: {
        GError *err = NULL; /* error to show to users                 */
        gchar *dbg = NULL;  /* additional debug string for developers */

        gst_message_parse_error(msg, &err, &dbg);
        if (err) {
            log_error("ERROR: %s\n", err->message);
            g_error_free(err);
        }
        if (dbg) {
            log_error("[Debug details: %s]\n", dbg);
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

    /* clean up */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));

    return ret;
}
