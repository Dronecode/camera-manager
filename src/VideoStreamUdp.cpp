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

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <unistd.h>

#include "VideoStreamUdp.h"
#include "log.h"

VideoStreamUdp::VideoStreamUdp(std::shared_ptr<CameraDevice> camDev)
    : mCamDev(camDev)
    , mState(STATE_IDLE)
    , mWidth(640)
    , mHeight(360)
    , mHost("127.0.0.1")
    , mPort(5600)
    , mOvText("")
    , mOvTime(30)
    , mOvFrmCnt(0)
    , mPipeline(nullptr)
    , mTextOverlay(nullptr)
{
    log_info("%s Device:%s", __func__, mCamDev->getDeviceId().c_str());

    mOvText = mCamDev->getDeviceId();
    mOvFrmCnt = mOvTime * 25;
}

VideoStreamUdp::~VideoStreamUdp()
{
}

int VideoStreamUdp::init()
{
    setState(STATE_INIT);
    return 0;
}

int VideoStreamUdp::uninit()
{
    setState(STATE_IDLE);
    return 0;
}

int VideoStreamUdp::getInfo(VideoStreamInfo &vidStreamInfo)
{
     log_debug("%s::%s", typeid(this).name(), __func__);
     vidStreamInfo.stream_id=1;
     vidStreamInfo.count=1;
     vidStreamInfo.type=0;
     vidStreamInfo.flags=0;
     vidStreamInfo.framerate=0;
     vidStreamInfo.resolution_h=0;
     vidStreamInfo.resolution_v=0;
     vidStreamInfo.bitrate=0;
     vidStreamInfo.rotation=0;
     vidStreamInfo.hfov=0;
     strncpy((char *)vidStreamInfo.name, "dummy", sizeof(vidStreamInfo.name));
     vidStreamInfo.name[sizeof(vidStreamInfo.name) - 1] = 0;
     strncpy((char *)vidStreamInfo.uri, "none", sizeof(vidStreamInfo.uri));
     vidStreamInfo.uri[sizeof(vidStreamInfo.uri) - 1] = 0;
    // vidStreamInfo.name="";
     // vidStreamInfo.uri="";
     return this->getState();
}



int VideoStreamUdp::start()
{
    log_info("%s::%s", typeid(this).name(), __func__);
    int ret = -1;
    ret = createAppsrcPipeline();
    setState(STATE_RUN);
    return ret;
}

int VideoStreamUdp::stop()
{
    log_info("%s::%s", typeid(this).name(), __func__);
    int ret = -1;
    ret = destroyAppsrcPipeline();
    setState(STATE_INIT);
    return ret;
}

int VideoStreamUdp::getState()
{
    return mState;
}

int VideoStreamUdp::setState(int state)
{
    // TODO :: Set state only as per FSM
    mState = state;
    return 0;
}

int VideoStreamUdp::setResolution(int imgWidth, int imgHeight)
{
    // TODO::Check if the argument is valid
    mWidth = imgWidth;
    mHeight = imgHeight;
    return 0;
}

int VideoStreamUdp::getResolution(int &imgWidth, int &imgHeight)
{
    imgWidth = mWidth;
    imgHeight = mHeight;
    return 0;
}

int VideoStreamUdp::setFormat(int vidFormat)
{
    // TODO::Check if the argument is valid
    return 0;
}

int VideoStreamUdp::getFormat()
{
    return 0;
}

int VideoStreamUdp::setAddress(std::string ipAddr)
{
    // TODO::Check if the argument is valid
    mHost = ipAddr;
    return 0;
}

std::string VideoStreamUdp::getAddress()
{
    return mHost;
}

int VideoStreamUdp::setPort(uint32_t port)
{
    // TODO::Check if the argument is valid
    mPort = port;
    return 0;
}

int VideoStreamUdp::getPort()
{
    return mPort;
}

int VideoStreamUdp::setTextOverlay(std::string text, int timeSec)
{
    mOvText = text;
    mOvTime = timeSec;
    // TODO :: Replace 25 with camera frame rate
    mOvFrmCnt = 25 * mOvTime;
    return 0;
}

std::string VideoStreamUdp::getTextOverlay()
{
    return mOvText;
}

GstBuffer *VideoStreamUdp::readFrame()
{
    GstBuffer *buffer;
    static GstClockTime timestamp = 0;
    CameraData data;
    CameraDevice::Status ret = mCamDev->read(data);
    if (ret == CameraDevice::Status::SUCCESS) {
        gsize size = data.bufSize;
        gsize offset = 0;
        gsize maxsize = size;
        buffer = gst_buffer_new_wrapped_full((GstMemoryFlags)0, data.buf, maxsize, offset, size,
                                             NULL, NULL);
    } else {
        log_error("Camera returned no frame");
        // TODO :: Change the multiplication factor based on pix format
        gsize size = mWidth * mHeight * 3;
        buffer = gst_buffer_new_allocate(NULL, size, NULL);
        // this makes the image white
        gst_buffer_memset(buffer, 0, 0xff, size);
    }

    // Add timestamp
    GST_BUFFER_PTS(buffer) = timestamp;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 25);
    timestamp += GST_BUFFER_DURATION(buffer);

    // Add Overlay
    std::string camText = mCamDev->getOverlayText();
    if (mOvText.compare(camText) != 0 && !camText.empty()) {
        mOvText = camText;
        mOvFrmCnt = 25 * mOvTime;
    }

    if (mOvFrmCnt || mOvFrmCnt == -1) {
        g_object_set(G_OBJECT(mTextOverlay), "text", mOvText.c_str(), "valignment", 2, "halignment",
                     0, "shaded-background", TRUE, NULL);
        if (mOvFrmCnt > 0)
            mOvFrmCnt--;
    } else {
        g_object_set(G_OBJECT(mTextOverlay), "text", "", NULL);
    }
    return buffer;
}

static void cb_need_data(GstAppSrc *appsrc, guint unused_size, gpointer user_data)
{
    GstFlowReturn ret;
    VideoStreamUdp *obj = (VideoStreamUdp *)user_data;

    GstBuffer *buffer = obj->readFrame();
    if (buffer) {
        g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
        if (ret != GST_FLOW_OK) {
            // some error
            log_error("Error in sending data to gst pipeline");
        }
    }
}

static void cb_enough_data(GstAppSrc *src, gpointer user_data)
{
}

static gboolean cb_seek_data(GstAppSrc *src, guint64 offset, gpointer user_data)
{
    return TRUE;
}

int VideoStreamUdp::createAppsrcPipeline()
{
    log_info("%s::%s", typeid(this).name(), __func__);

    int ret = 0;
    gboolean link_ok;
    GstElement *src, *conv, *enc, *parser, *payload, *sink;
    GstCaps *caps;

    mPipeline = gst_pipeline_new("UdpStream");
    src = gst_element_factory_make("appsrc", "VideoSrc");
    conv = gst_element_factory_make("videoconvert", "Conv");
    mTextOverlay = gst_element_factory_make("textoverlay", "textoverlay");
    enc = gst_element_factory_make("x264enc", "H264Enc");
    parser = gst_element_factory_make("h264parse", "Parser");
    payload = gst_element_factory_make("rtph264pay", "H264Rtp");
    sink = gst_element_factory_make("udpsink", "UdpSink");

    // TODO::Check if all the elements are created
    if (!mPipeline || !src || !conv || !mTextOverlay || !enc || !parser || !payload || !sink) {
        log_error("One element could not be created. Exiting.\n");
        return -1;
    }

    // Set appsrc caps
    // TODO :: Remove the format hardcode
    gst_app_src_set_caps(GST_APP_SRC(src),
                         gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB", "width",
                                             G_TYPE_INT, mWidth, "height", G_TYPE_INT, mHeight,
                                             "framerate", GST_TYPE_FRACTION, 25, 1, NULL));

    // Setup appsrc
    g_object_set(G_OBJECT(src), "is-live", TRUE, "format", GST_FORMAT_TIME, NULL);

    // Setup convertor
    caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "I420", NULL);

    // Setup encoder

    // Setup payload

    // Setup sink
    g_object_set(G_OBJECT(sink), "host", mHost.c_str(), NULL);
    g_object_set(G_OBJECT(sink), "port", mPort, NULL);

    // Add element to bin
    // gst_bin_add_many(GST_BIN(mPipeline), src, conv, enc, parser, payload, sink, NULL);
    gst_bin_add_many(GST_BIN(mPipeline), src, conv, mTextOverlay, enc, parser, payload, sink, NULL);

    // Link src to sink
    gst_element_link_many(src, conv, NULL);
    link_ok = gst_element_link_filtered(conv, mTextOverlay, caps);
    if (!link_ok) {
        log_error("Failed to link convertor and encoder!");
    }
    gst_element_link_many(mTextOverlay, enc, payload, sink, NULL);

    // Connect signals
    GstAppSrcCallbacks cbs;
    cbs.need_data = cb_need_data;
    cbs.enough_data = cb_enough_data;
    cbs.seek_data = cb_seek_data;
    gst_app_src_set_callbacks(GST_APP_SRC_CAST(src), &cbs, this, NULL);

    // Set pipeline to play
    gst_element_set_state(mPipeline, GST_STATE_PLAYING);

    return ret;
}

int VideoStreamUdp::destroyAppsrcPipeline()
{
    log_info("%s::%s", typeid(this).name(), __func__);

    int ret = 0;

    // clean up
    gst_element_set_state(mPipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(mPipeline));

    return ret;
}
