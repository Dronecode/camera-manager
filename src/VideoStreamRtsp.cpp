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

#include "VideoStreamRtsp.h"

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 360
#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_SERVICE_PORT 8554

VideoStreamRtsp::VideoStreamRtsp(std::shared_ptr<CameraDevice> camDev)
    : mCamDev(camDev)
    , mState(STATE_IDLE)
    , mWidth(DEFAULT_WIDTH)
    , mHeight(DEFAULT_HEIGHT)
    , mHost(DEFAULT_HOST)
    , mPort(DEFAULT_SERVICE_PORT)
    , mServer(nullptr)
    , mPipeline(nullptr)
{
    log_info("%s Device:%s", __func__, mCamDev->getDeviceId().c_str());
    mPath = "/" + mCamDev->getDeviceId();
    if (mPath.find("gazebo") != std::string::npos)
        mPath = "/gazebo";
}

VideoStreamRtsp::~VideoStreamRtsp()
{
    log_debug("%s::%s", typeid(this).name(), __func__);

    stop();
    uninit();
}

int VideoStreamRtsp::init()
{
    if (getState() != STATE_IDLE) {
        log_error("Invalid State : %d", getState());
        return -1;
    }

    setState(STATE_INIT);
    return 0;
}

int VideoStreamRtsp::uninit()
{
    if (getState() == STATE_IDLE)
        return 0;

    if (getState() != STATE_INIT && getState() != STATE_ERROR) {
        log_warning("Invalid State : %d", getState());
        return -1;
    }

    setState(STATE_IDLE);
    return 0;
}

int VideoStreamRtsp::start()
{
    log_debug("%s::%s", typeid(this).name(), __func__);
    int ret = 0;

    // check state
    if (getState() != STATE_INIT) {
        log_error("Invalid State : %d", getState());
        return -1;
    }

    ret = startRtspServer();
    if (!ret) {
        setState(STATE_RUN);
    }

    return ret;
}

int VideoStreamRtsp::stop()
{
    log_debug("%s::%s", typeid(this).name(), __func__);
    int ret = 0;

    // check state
    if (getState() != STATE_RUN && getState() != STATE_ERROR) {
        return -1;
    }

    ret = stopRtspServer();
    setState(STATE_INIT);
    return ret;
}

int VideoStreamRtsp::getState()
{
    return mState;
}

/*
 * IDLE-> INIT-> RUN-> INIT-> IDLE
 */
int VideoStreamRtsp::setState(int state)
{
    int ret = 0;
    // log_debug("%s : %d", __func__, state);

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

int VideoStreamRtsp::setResolution(int imgWidth, int imgHeight)
{
    mWidth = imgWidth;
    mHeight = imgHeight;
    return 0;
}

int VideoStreamRtsp::getResolution(int &imgWidth, int &imgHeight)
{
    imgWidth = mWidth;
    imgHeight = mHeight;
    return 0;
}

int VideoStreamRtsp::setFormat(int vidFormat)
{
    return 0;
}

int VideoStreamRtsp::getFormat()
{
    return 0;
}

/* The host/IP/Multicast group to send the packets to */
int VideoStreamRtsp::setAddress(std::string ipAddr)
{
    return 0;
}

std::string VideoStreamRtsp::getAddress()
{
    return 0;
}

/* The port to send the packets to */
int VideoStreamRtsp::setPort(uint32_t port)
{
    mPort = port;
    return 0;
}

int VideoStreamRtsp::getPort()
{
    return mPort;
}

GstBuffer *VideoStreamRtsp::readFrame()
{
    log_debug("%s::%s", typeid(this).name(), __func__);

    GstBuffer *buffer;
    static GstClockTime timestamp = 0;
    CameraData data;
    CameraDevice::Status ret = CameraDevice::Status::ERROR_UNKNOWN;
    ret = mCamDev->read(data);
    if (ret == CameraDevice::Status::SUCCESS) {
        gsize size = data.bufSize;
        gsize offset = 0;
        gsize maxsize = size;
        buffer = gst_buffer_new_wrapped_full((GstMemoryFlags)0, data.buf, maxsize, offset, size,
                                             NULL, NULL);
    } else {
        log_error("Camera returned no frame");
        /* TODO :: Change the multiplication factor based on pix format */
        gsize size = mWidth * mHeight * 3;
        buffer = gst_buffer_new_allocate(NULL, size, NULL);
        /* this makes the image white */
        gst_buffer_memset(buffer, 0, 0xff, size);
    }

    /* Add timestamp */
    GST_BUFFER_PTS(buffer) = timestamp;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 25);
    timestamp += GST_BUFFER_DURATION(buffer);

    return buffer;
}

/* called when we need to give data to appsrc */
static void need_data(GstElement *appsrc, guint unused, gpointer user_data)
{
    GstFlowReturn ret;
    VideoStreamRtsp *obj = reinterpret_cast<VideoStreamRtsp *>(user_data);

    GstBuffer *buffer = obj->readFrame();
    if (buffer) {
        g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
        if (ret != GST_FLOW_OK) {
            /* some error */
            log_error("Error in sending data to gst pipeline");
        }
    }
}

/* called when a new media pipeline is constructed. We can query the
 * pipeline and configure our appsrc */
static void rtspMediaConfigure(GstRTSPMediaFactory *factory, GstRTSPMedia *media,
                               gpointer user_data)
{
    log_info("%s", __func__);

    VideoStreamRtsp *obj = reinterpret_cast<VideoStreamRtsp *>(user_data);

    GstElement *element, *appsrc;
    int width, height;

    obj->getResolution(width, height);
    obj->getPort();

    /* get the element used for providing the streams of the media */
    element = gst_rtsp_media_get_element(media);

    /* get our appsrc, we named it 'mysrc' with the name property */
    appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "mysrc");

    /* this instructs appsrc that we will be dealing with timed buffer */
    gst_util_set_object_arg(G_OBJECT(appsrc), "format", "time");

    /* configure the caps of the video */
    g_object_set(G_OBJECT(appsrc), "caps",
                 gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB", "width",
                                     G_TYPE_INT, width, "height", G_TYPE_INT, height, "framerate",
                                     GST_TYPE_FRACTION, 0, 1, NULL),
                 NULL);

    /* install the callback that will be called when a buffer is needed */
    g_signal_connect(appsrc, "need-data", (GCallback)need_data, user_data);
    gst_object_unref(appsrc);
    gst_object_unref(element);
}

std::string VideoStreamRtsp::rtspPipelineName()
{
    std::string source;
    std::string name;

    if (mCamDev->isGstV4l2Src()) {
        source = "v4l2src device=/dev/" + mCamDev->getDeviceId();
    } else {
        source = "appsrc name=mysrc";
    }

    /* TODO:: format, encoder elements should be configurable */
    name = source + " ! videoconvert ! video/x-raw, format=I420 ! x264enc ! rtph264pay name=pay0";

    log_debug("%s:%s", __func__, name.c_str());
    return name;
}

int VideoStreamRtsp::startRtspServer()
{
    log_debug("%s::%s", typeid(this).name(), __func__);

    gst_init(nullptr, nullptr);

    /* create RTSP server */
    mServer = gst_rtsp_server_new();

    /* set the port number */
    g_object_set(mServer, "service", std::to_string(mPort).c_str(), nullptr);

    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();
    if (!factory) {
        log_error("Error in creating media factory");
        return -1;
    }

    gst_rtsp_media_factory_set_launch(factory, rtspPipelineName().c_str());

    if (!mCamDev->isGstV4l2Src()) {
        /* notify when our media is ready, This is called whenever someone asks for
         * the media and a new pipeline with our appsrc is created */
        g_signal_connect(factory, "media-configure", (GCallback)rtspMediaConfigure, this);
    }

    /* get the default mount points from the server */
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(mServer);

    /* attach the video to the RTSP URL */
    gst_rtsp_mount_points_add_factory(mounts, mPath.c_str(), factory);
    log_info("RTSP stream ready at rtsp://127.0.0.1:%s%s\n", std::to_string(mPort).c_str(),
             mPath.c_str());
    g_object_unref(mounts);

    gst_rtsp_server_attach(mServer, nullptr);

    return 0;
}

int VideoStreamRtsp::stopRtspServer()
{
    log_debug("%s::%s", typeid(this).name(), __func__);

    g_object_unref(mServer);

    return 0;
}
