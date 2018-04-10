/*
 * This file is part of the Camera Streaming Daemon
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
#include <gst/gst.h>
#include <sstream>

#include "VideoCaptureGst.h"
#include "log.h"

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480
#define DEFAULT_BITRATE 512
#define DEFAULT_FRAMERATE 25
#define DEFAULT_ENCODER CameraParameters::VIDEO_CODING_AVC
#define DEFAULT_FILE_FORMAT CameraParameters::VIDEO_FILE_MP4
#define DEFAULT_FILE_PATH "/tmp/"

int VideoCaptureGst::vidCount = 0;

VideoCaptureGst::VideoCaptureGst(std::shared_ptr<CameraDevice> camDev)
    : mCamDev(camDev)
    , mState(STATE_IDLE)
    , mWidth(0)
    , mHeight(0)
    , mBitRate(0)
    , mFrmRate(0)
    , mEnc(DEFAULT_ENCODER)
    , mFileFmt(DEFAULT_FILE_FORMAT)
    , mFilePath(DEFAULT_FILE_PATH)
    , mPipeline(nullptr)
{
    log_info("%s Device:%s", __func__, mCamDev->getDeviceId().c_str());
}

VideoCaptureGst::VideoCaptureGst(std::shared_ptr<CameraDevice> camDev,
                                 struct VideoSettings &vidSetting)
    : mCamDev(camDev)
    , mState(STATE_IDLE)
    , mWidth(vidSetting.width)
    , mHeight(vidSetting.height)
    , mBitRate(vidSetting.bitRate)
    , mFrmRate(vidSetting.frameRate)
    , mEnc(vidSetting.encoder)
    , mFileFmt(vidSetting.fileFormat)
    , mFilePath(DEFAULT_FILE_PATH)
    , mPipeline(nullptr)

{
    log_info("%s Device:%s with settings", __func__, mCamDev->getDeviceId().c_str());
}

VideoCaptureGst::~VideoCaptureGst()
{
    if (getState() != STATE_IDLE) {
        stop();
        uninit();
    }
}

int VideoCaptureGst::init()
{
    log_info("%s::%s", typeid(this).name(), __func__);

    if (getState() != STATE_IDLE) {
        log_error("Invalid State : %d", getState());
        return -1;
    }

    setState(STATE_INIT);
    return 0;
}

int VideoCaptureGst::uninit()
{
    log_info("%s::%s", typeid(this).name(), __func__);

    if (getState() != STATE_INIT && getState() != STATE_ERROR) {
        log_error("Invalid State : %d", getState());
        return -1;
    }

    setState(STATE_IDLE);
    return 0;
}

int VideoCaptureGst::start()
{
    log_info("%s::%s", typeid(this).name(), __func__);

    if (getState() != STATE_INIT) {
        log_error("Invalid State : %d", getState());
        return -1;
    }

    // TODO::Validate video settings

    int ret = 0;
    if (mCamDev->isGstV4l2Src()) {
        ret = createV4l2Pipeline();
        if (!ret)
            setState(STATE_RUN);
        else
            setState(STATE_ERROR);
    } else
        ret = -1;

    return ret;
}

int VideoCaptureGst::stop()
{
    log_info("%s::%s", typeid(this).name(), __func__);

    if (getState() != STATE_RUN) {
        log_error("Invalid State : %d", getState());
        return -1;
    }

    destroyPipeline();

    setState(STATE_INIT);
    return 0;
}

int VideoCaptureGst::setState(int state)
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

int VideoCaptureGst::getState()
{
    return mState;
}

int VideoCaptureGst::setResolution(int vidWidth, int vidHeight)
{
    int ret = 0;

    if (getState() == STATE_RUN)
        log_warning("Change will not take effect");

    mWidth = vidWidth;
    mHeight = vidHeight;

    return ret;
}

int VideoCaptureGst::getResolution(int &vidWidth, int &vidHeight)
{
    int ret = 0;

    if (getState() == STATE_RUN)
        log_warning("Change will not take effect");

    vidWidth = mWidth;
    vidHeight = mHeight;

    return ret;
}

int VideoCaptureGst::setBitRate(int bitRate)
{
    int ret = 0;

    if (getState() == STATE_RUN)
        log_warning("Change will not take effect");

    mBitRate = bitRate;

    return ret;
}

int VideoCaptureGst::setFrameRate(int frameRate)
{
    int ret = 0;

    if (getState() == STATE_RUN)
        log_warning("Change will not take effect");

    mFrmRate = frameRate;

    return ret;
}

int VideoCaptureGst::setEncoder(CameraParameters::VIDEO_CODING_FORMAT vidEnc)
{
    int ret = 0;

    if (getState() == STATE_RUN)
        log_warning("Change will not take effect");

    mEnc = vidEnc;

    return ret;
}

int VideoCaptureGst::setFormat(CameraParameters::VIDEO_FILE_FORMAT fileFormat)
{
    int ret = 0;

    if (getState() == STATE_RUN)
        log_warning("Change will not take effect");

    mFileFmt = fileFormat;

    return ret;
}

int VideoCaptureGst::setLocation(const std::string vidPath)
{
    int ret = 0;

    if (getState() == STATE_RUN)
        log_warning("Change will not take effect");

    mFilePath = vidPath;

    return ret;
}

std::string VideoCaptureGst::getLocation()
{
    return mFilePath;
}

std::string VideoCaptureGst::getGstEncName(int encFormat)
{
    std::string ret;

    switch (encFormat) {
    case CameraParameters::VIDEO_CODING_AVC:
        ret = std::string("x264enc");
        break;
    default:
        ret = {};
        break;
    }

    return ret;
}

std::string VideoCaptureGst::getGstParserName(int encFormat)
{
    std::string ret;

    switch (encFormat) {
    case CameraParameters::VIDEO_CODING_AVC:
        ret = std::string("h264parse");
        break;
    default:
        ret = {};
        break;
    }

    return ret;
}

std::string VideoCaptureGst::getGstMuxerName(int fileFormat)
{
    std::string ret;

    switch (fileFormat) {
    case CameraParameters::VIDEO_FILE_MP4:
        ret = std::string("mp4mux");
        break;
    default:
        ret = {};
        break;
    }

    return ret;
}

std::string VideoCaptureGst::getFileExt(int fileFormat)
{
    std::string ret;

    switch (fileFormat) {
    case CameraParameters::VIDEO_FILE_MP4:
        ret = std::string("mp4");
        break;
    default:
        ret = {};
        break;
    }

    return ret;
}

std::string VideoCaptureGst::getGstV4l2PipelineName()
{
    std::string device = mCamDev->getDeviceId();
    if (device.empty())
        return {};

    std::string encoder = getGstEncName(mEnc);
    std::string parser = getGstParserName(mEnc);
    std::string muxer = getGstMuxerName(mFileFmt);
    std::string ext = getFileExt(mFileFmt);
    if (encoder.empty() || parser.empty() || muxer.empty() || ext.empty())
        return {};

    std::stringstream filter;
    std::stringstream sbr;
    std::stringstream ss;

    filter << "video/x-raw, ";
    if (mFrmRate > 0)
        filter << " framerate=" << std::to_string(mFrmRate) << "/1,";
    if (mWidth > 0 && mHeight > 0)
        filter << " width=" << std::to_string(mWidth) << ", height=" << std::to_string(mHeight);

    if (mBitRate > 0)
        sbr << " bitrate=" << std::to_string(mBitRate);

    ss << "v4l2src device=" << device << " ! " << filter.str() << " ! " << encoder << sbr.str()
       << " ! " << parser << " ! " << muxer << " ! "
       << "filesink location=" << mFilePath + "vid_" << std::to_string(++vidCount) << "." + ext;

    return ss.str();
}

static gboolean gstMsgCb(GstBus *bus, GstMessage *message, gpointer user_data)
{
    GstElement *pipeline = (GstElement *)user_data;
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR: {
        GError *err = NULL;
        gchar *name, *debug = NULL;

        name = gst_object_get_path_string(message->src);
        gst_message_parse_error(message, &err, &debug);

        log_error("ERROR: from element %s: %s\n", name, err->message);
        if (debug != NULL)
            log_error("Additional debug info:\n%s\n", debug);

        g_error_free(err);
        g_free(debug);
        g_free(name);

        // TODO :: stop the pipeline
        // Update video cap status
        break;
    }
    case GST_MESSAGE_WARNING: {
        GError *err = NULL;
        gchar *name, *debug = NULL;

        name = gst_object_get_path_string(message->src);
        gst_message_parse_warning(message, &err, &debug);

        log_error("ERROR: from element %s: %s\n", name, err->message);
        if (debug != NULL)
            log_error("Additional debug info:\n%s\n", debug);

        g_error_free(err);
        g_free(debug);
        g_free(name);
        break;
    }
    case GST_MESSAGE_EOS: {
        log_info("Got EOS\n");
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        break;
    }
    default:
        break;
    }

    return TRUE;
}

int VideoCaptureGst::createV4l2Pipeline()
{
    int ret = 0;
    log_info("%s", __func__);

    GError *error = nullptr;
    GstStateChangeReturn result;

    std::string pipeline_str = getGstV4l2PipelineName();
    if (pipeline_str.empty()) {
        log_error("Pipeline String error");
        return 1;
    }
    log_debug("pipeline = %s", pipeline_str.c_str());

    mPipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (!mPipeline) {
        log_error("Error creating pipeline");
        if (error)
            g_clear_error(&error);
        return 1;
    }
    result = gst_element_set_state(mPipeline, GST_STATE_PLAYING);
    if (result == GST_STATE_CHANGE_FAILURE) {
        log_error("Error setting PLAY state");
        gst_object_unref(GST_OBJECT(mPipeline));
        mPipeline = nullptr;
        return 1;
    }

    result = gst_element_get_state(mPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
    if (result != GST_STATE_CHANGE_SUCCESS) {
        log_error("Error going to PLAY state");
        gst_object_unref(GST_OBJECT(mPipeline));
        mPipeline = nullptr;
        return 1;
    }

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(mPipeline));
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message", G_CALLBACK(gstMsgCb), mPipeline);
    gst_object_unref(GST_OBJECT(bus));

    return ret;
}

int VideoCaptureGst::destroyPipeline()
{
    int ret = 0;
    log_info("%s", __func__);

    // gst_element_set_state (mPipeline, GST_STATE_NULL);
    // gst_object_unref (mPipeline);
    log_info("Sending EoS");
    gst_element_send_event(mPipeline, gst_event_new_eos());

    return ret;
}
