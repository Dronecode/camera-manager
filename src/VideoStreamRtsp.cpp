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

#include "VideoStreamRtsp.h"

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_SERVICE_PORT 8554

GstRTSPServer *VideoStreamRtsp::mServer = nullptr;
bool VideoStreamRtsp::isAttach = false;
uint32_t VideoStreamRtsp::refCnt = 0;

static std::string getGstVideoConvertor()
{

    std::string convertor;

    convertor = "videoconvert";

    return convertor;
}

static std::string getGstVideoConvertorCaps(std::map<std::string, std::string> &params,
                                            uint32_t setWidth, uint32_t setHeight)
{
    std::string caps;

    /* output pixel format is fixed as QGC only supports I420*/
    caps = "video/x-raw, format=I420";

    std::string width = params["width"];
    std::string height = params["height"];

    /*
     * RTSP Video Stream resolution
     * 1. Set by query string in URL
     * 2. Set by client using VideoStream API
     * 3. Default - Camera Device Resolution
     *
     */

    if (!width.empty() && !height.empty()) {
        caps = caps + ", width=" + width + ", height=" + height;
    } else if (setWidth != 0 && setHeight != 0) {
        caps = caps + ", width=" + std::to_string(setWidth) + ", height="
            + std::to_string(setHeight);
    }

    return caps;
}

static std::string getGstVideoEncoder(CameraParameters::VIDEO_CODING_FORMAT encFormat)
{
    std::string enc;

    switch (encFormat) {
    case CameraParameters::VIDEO_CODING_AVC:
        enc = std::string("vaapih264enc");
        break;
    default:
        enc = std::string("vaapih264enc");
        break;
    }

    return enc;
}

static std::string getGstRtspVideoSink()
{
    std::string sink;

    sink = "rtph264pay name=pay0";

    return sink;
}

static std::string getGstPixFormat(CameraParameters::PixelFormat pixFormat)
{
    std::string pix;

    switch (pixFormat) {
    case CameraParameters::PixelFormat::PIXEL_FORMAT_RGB24:
        pix = "RGB";
        break;
    case CameraParameters::PixelFormat::PIXEL_FORMAT_UYVY:
        pix = "UYVY";
        break;
    default:
        pix = "I420";
    }

    return pix;
}

static float getBytesPerPixel(CameraParameters::PixelFormat pixFormat)
{
    float ret = 2;

    switch (pixFormat) {
    case CameraParameters::PixelFormat::PIXEL_FORMAT_RGB24:
        ret = 3;
        break;
    case CameraParameters::PixelFormat::PIXEL_FORMAT_UYVY:
        ret = 2;
        break;
    default:
        ret = 2;
    }

    return ret;
}

static void addParam(std::map<std::string, std::string> &map, const std::string &param)
{
    size_t j = param.find('=');
    if (j == std::string::npos)
        return;

    map[param.substr(0, j)] = param.substr(j + 1);
}

static std::map<std::string, std::string> parseUrlQuery(const char *query)
{
    std::map<std::string, std::string> map;

    if (!query || !query[0])
        return map;

    std::string query_str = query;
    size_t i = 0, j;

    j = query_str.find('&');
    while (j != std::string::npos) {
        addParam(map, query_str.substr(i, j - i));
        i = j + 1;
        j = query_str.find('&', j + 1);
    }
    addParam(map, query_str.substr(i, j - i));

    return map;
}

VideoStreamRtsp::VideoStreamRtsp(std::shared_ptr<CameraDevice> camDev)
    : mCamDev(camDev)
    , mState(STATE_IDLE)
    , mWidth(0)
    , mHeight(0)
    , mEncFormat(CameraParameters::VIDEO_CODING_AVC)
    , mHost(DEFAULT_HOST)
    , mPort(DEFAULT_SERVICE_PORT)
{
    log_info("%s Device:%s", __func__, mCamDev->getDeviceId().c_str());
    mPath = "/" + mCamDev->getDeviceId();
    if (mPath.find("gazebo") != std::string::npos)
        mPath = "/gazebo";

    /* Default: Set the RTSP video res same as camera res */
    mCamDev->getSize(mWidth, mHeight);
}

VideoStreamRtsp::~VideoStreamRtsp()
{
    log_debug("%s::%s", __func__, mPath.c_str());

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

int VideoStreamRtsp::setPort(uint32_t port)
{
    mPort = port;
    return 0;
}

int VideoStreamRtsp::getPort()
{
    return mPort;
}

int VideoStreamRtsp::getCameraResolution(uint32_t &width, uint32_t &height)
{
    mCamDev->getSize(width, height);
    return 0;
}

CameraParameters::PixelFormat VideoStreamRtsp::getCameraPixelFormat()
{
    CameraParameters::PixelFormat format;
    mCamDev->getPixelFormat(format);
    return format;
}

std::string VideoStreamRtsp::getGstPipeline(std::map<std::string, std::string> &params)
{
    std::string name;
    std::string source;
    if (mCamDev->isGstV4l2Src()) {
        source = "v4l2src device=/dev/" + mCamDev->getDeviceId();
    } else {
        source = "appsrc name=mysrc";
    }

    name = source + " ! " + getGstVideoConvertor() + " ! "
        + getGstVideoConvertorCaps(params, mWidth, mHeight) + " ! " + getGstVideoEncoder(mEncFormat)
        + " ! " + getGstRtspVideoSink();

    log_debug("%s:%s", __func__, name.c_str());
    return name;
}

GstBuffer *VideoStreamRtsp::readFrame()
{
    // log_debug("%s::%s", typeid(this).name(), __func__);

    GstBuffer *buffer;
    CameraData data;
    CameraDevice::Status ret = CameraDevice::Status::ERROR_UNKNOWN;
    ret = mCamDev->read(data);
    /* add retry logic? */
    if (ret == CameraDevice::Status::SUCCESS) {
        gsize size = data.bufSize;
        gsize offset = 0;
        gsize maxsize = size;
        buffer = gst_buffer_new_wrapped_full((GstMemoryFlags)0, data.buf, maxsize, offset, size,
                                             NULL, NULL);
    } else {
        log_error("Camera returned no frame");
        uint32_t width, height;
        getCameraResolution(width, height);
        gsize size = width * height * getBytesPerPixel(getCameraPixelFormat());
        buffer = gst_buffer_new_allocate(NULL, size, NULL);
        /* this makes the image white */
        gst_buffer_memset(buffer, 0, 0xff, size);
    }

    return buffer;
}

/* called when we need to give data to appsrc */
static void cb_need_data(GstAppSrc *appsrc, guint unused, gpointer user_data)
{
    GstFlowReturn ret;
    VideoStreamRtsp *obj = reinterpret_cast<VideoStreamRtsp *>(user_data);

    GstBuffer *buffer = obj->readFrame();
    if (buffer) {
        ret = gst_app_src_push_buffer(appsrc, buffer);
        if (ret != GST_FLOW_OK) {
            /* some error */
            log_error("Error in sending data to gst pipeline");
        }
    }
}

/*
 * ### For future reference ###
 * After setup request, gst-rtsp-server does following to construct media and pipeline
 * 1. Convert the URL to a key : Callback gen_key()
 * 2. Construct the media : Callback construct()
 * 2a. Create element : Callback create_element()
 * 2b. Create pipeline : Callback create_pipeline()
 * 5. Emit Signal: media_constructed
 * 6. Configure media : Callback configure()
 * 7. Emit Signal: media_configure
 */

static GstElement *cb_create_element(GstRTSPMediaFactory *factory, const GstRTSPUrl *url)
{
    log_debug("%s", __func__);
    log_info("host:%s,port:%d,abspath:%s,query:%s", url->host, url->port, url->abspath, url->query);

    VideoStreamRtsp *obj
        = reinterpret_cast<VideoStreamRtsp *>(g_object_get_data(G_OBJECT(factory), "user_data"));

    /* parse query string from URL */
    std::map<std::string, std::string> params = parseUrlQuery(url->query);

    std::string launch = obj->getCameraDevice()->getGstRTSPPipeline();
    if (launch.empty()) {
        /* build pipeline description based on params received from URL */
        launch = obj->getGstPipeline(params);
    }

    log_info("GST Pipeline: %s", launch.c_str());

    GError *error = NULL;
    GstElement *pipeline = NULL;
    /* create new pipeline */
    pipeline = gst_parse_launch(launch.c_str(), &error);
    if (pipeline == NULL) {
        if (error)
            g_error_free(error);
        return NULL;
    }

    if (error != NULL) {
        /* a recoverable error was encountered */
        log_warning("recoverable parsing error: %s", error->message);
        g_error_free(error);
    }

    /* return if not appsrc pipeline, else configure */
    if (launch.find("appsrc") == std::string::npos)
        return pipeline;

    /* configure the appsrc element*/
    GstElement *appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "mysrc");

    uint32_t width, height;
    obj->getCameraResolution(width, height);
    std::string fmt = getGstPixFormat(obj->getCameraPixelFormat());

    /* set capabilities of appsrc element*/
    gst_app_src_set_caps(GST_APP_SRC(appsrc),
                         gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, fmt.c_str(),
                                             "width", G_TYPE_INT, width, "height", G_TYPE_INT,
                                             height, "framerate", GST_TYPE_FRACTION, 25, 1, NULL));

    /* setup appsrc */
    g_object_set(G_OBJECT(appsrc), "stream-type", 0, "format", GST_FORMAT_TIME, "is-live", TRUE,
                 NULL);

    /* install the callback that will be called when a buffer is needed */
    GstAppSrcCallbacks cbs;
    cbs.need_data = cb_need_data;
    cbs.enough_data = NULL;
    cbs.seek_data = NULL;
    gst_app_src_set_callbacks(GST_APP_SRC_CAST(appsrc), &cbs, obj, NULL);

    gst_object_unref(appsrc);

    return pipeline;
}

static void cb_unprepared(GstRTSPMedia *media, gpointer user_data)
{
    log_debug("%s", __func__);

    /* TODO:: Stop camera device capturing*/
}

static void cb_media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media)
{
    log_debug("%s", __func__);

    /* TODO:: Start camera device capturing */

    g_signal_connect(media, "unprepared", (GCallback)cb_unprepared,
                     g_object_get_data(G_OBJECT(factory), "user_data"));
}

int VideoStreamRtsp::startRtspServer()
{
    log_debug("%s::%s", typeid(this).name(), __func__);

    /* create RTSP server */
    createRtspServer();

    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();
    if (!factory) {
        log_error("Error in creating media factory");
        return -1;
    }

    g_object_set_data(G_OBJECT(factory), "user_data", this);
    GstRTSPMediaFactoryClass *factory_class = GST_RTSP_MEDIA_FACTORY_GET_CLASS(factory);
    factory_class->create_element = cb_create_element;

    g_signal_connect(factory, "media-configure", (GCallback)cb_media_configure, NULL);

    /* get the default mount points from the server */
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(mServer);

    /* attach the video to the RTSP URL */
    gst_rtsp_mount_points_add_factory(mounts, mPath.c_str(), factory);
    log_info("RTSP stream ready at rtsp://<ip-address>:%s%s\n", std::to_string(mPort).c_str(),
             mPath.c_str());
    g_object_unref(mounts);

    /* Attach RTSP Server */
    attachRtspServer();

    return 0;
}

int VideoStreamRtsp::stopRtspServer()
{
    log_debug("%s::%s", typeid(this).name(), __func__);

    /* get the default mount points from the server */
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(mServer);

    /* remove the media factory associated with path in mounts*/
    gst_rtsp_mount_points_remove_factory(mounts, mPath.c_str());

    g_object_unref(mounts);

    /* Destroy RTSP server */
    destroyRtspServer();

    return 0;
}

GstRTSPServer *VideoStreamRtsp::createRtspServer()
{
    if (mServer) {
        refCnt++;
        return mServer;
    } else {
        log_info("%s", __func__);
        gst_init(nullptr, nullptr);

        /* create RTSP server */
        mServer = gst_rtsp_server_new();

        /* set the port number */
        g_object_set(mServer, "service", std::to_string(mPort).c_str(), nullptr);
        refCnt++;
        return mServer;
    }
}

void VideoStreamRtsp::destroyRtspServer()
{
    refCnt--;
    if (refCnt == 0) {
        log_info("%s", __func__);
        g_object_unref(mServer);
        mServer = nullptr;
        isAttach = false;
    }

    return;
}

static gboolean timeout(GstRTSPServer *server)
{
    GstRTSPSessionPool *pool;

    pool = gst_rtsp_server_get_session_pool(server);
    gst_rtsp_session_pool_cleanup(pool);
    g_object_unref(pool);

    return TRUE;
}

void VideoStreamRtsp::attachRtspServer()
{
    if (!isAttach) {
        log_info("%s", __func__);
        gst_rtsp_server_attach(mServer, nullptr);
        isAttach = true;

        /* Periodically remove timed out sessions*/
        g_timeout_add_seconds(2, (GSourceFunc)timeout, mServer);
    }
}
