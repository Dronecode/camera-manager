/*
 * This file is part of the Dronecode Camera Manager
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
#include <fcntl.h>
#include <gst/app/gstappsrc.h>
#include <linux/videodev2.h>
#include <malloc.h>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>

#include "gstreamer_pipeline_builder.h"
#include "log.h"
#include "stream_aero_bottom.h"

#define BOTTOM_DEVICE "/dev/video2"

#define WIDTH (640)
#define HEIGHT (480)
#define BUFS_COUNT (4)

struct Context {
    int fd;
    void *bufs[BUFS_COUNT];
    size_t bufs_len[BUFS_COUNT];
    struct v4l2_buffer last_buf;
};

static void _release_last_buffer(Context *ctx)
{
    if (ctx->last_buf.type) {
        ioctl(ctx->fd, VIDIOC_QBUF, &ctx->last_buf);
        ctx->last_buf.type = 0;
    }
}

static GstBuffer *_read_frame(Context *ctx)
{
    struct v4l2_buffer buf {};
    fd_set fds;
    struct timeval tv;
    int r;

    _release_last_buffer(ctx);

    FD_ZERO(&fds);
    FD_SET(ctx->fd, &fds);

    tv.tv_sec = 4;
    tv.tv_usec = 0;

    r = select(ctx->fd + 1, &fds, NULL, NULL, &tv);
    if (r <= 0)
        return nullptr;

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_USERPTR;
    if (ioctl(ctx->fd, VIDIOC_DQBUF, &buf) < 0)
        return nullptr;

    ctx->last_buf = buf;

    return gst_buffer_new_wrapped_full((GstMemoryFlags)0, (void*)buf.m.userptr, buf.length, 0, buf.length, NULL, NULL);
}

static void cb_need_data(GstAppSrc *appsrc, guint unused_size, gpointer user_data)
{
    GstFlowReturn ret;
    Context *ctx = (Context *)user_data;
    assert(ctx);

    GstBuffer *buffer = _read_frame(ctx);
    if (buffer)
        g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
}

static void cb_enough_data(GstAppSrc *src, gpointer user_data)
{
}

static gboolean cb_seek_data(GstAppSrc *src, guint64 offset, gpointer user_data)
{
    return TRUE;
}

StreamAeroBottom::StreamAeroBottom(const char *path, const char *name)
    : Stream()
    , _path(path)
    , _name(name)
{
    PixelFormat f{Stream::fourCC("NV12")};
    f.frame_sizes.emplace_back(FrameSize{WIDTH, HEIGHT});
    formats.push_back(f);
}

const std::string StreamAeroBottom::get_path() const
{
    return _path;
}

const std::string StreamAeroBottom::get_name() const
{
    return _name;
}

static std::string create_pipeline(std::map<std::string, std::string> &params)
{
    std::stringstream ss;
    GstreamerPipelineBuilder &gst = GstreamerPipelineBuilder::get_instance();

    ss << "appsrc name=mysource ! videoconvert ! video/x-raw,width=" << WIDTH << ",height=" << HEIGHT <<",format=NV12";

    //Croping the source image because bottom camera has a noise when set in VGA resolution
    ss << "! videocrop top=0 left=0 right=40 bottom=10";
    ss << " ! " << gst.create_encoder(params) << " ! " << gst.create_muxer(params) << " name=pay0";

    return ss.str();
}

static void _device_deinit(Context *ctx)
{
    for (int i = 0; i < BUFS_COUNT; i++) {
        free(ctx->bufs[i]);
        ctx->bufs[i] = nullptr;
    }
    _release_last_buffer(ctx);
    close(ctx->fd);
    ctx->fd = -1;
}

static bool _device_init(Context *ctx)
{
    struct v4l2_streamparm parm {};
    struct v4l2_requestbuffers req {};
    struct v4l2_format fmt {};
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int channel = 1;
    unsigned int page_size, buffer_size;

    ctx->fd = ::open(BOTTOM_DEVICE, O_RDWR | O_NONBLOCK, 0);
    if (ctx->fd < 0) {
        log_error("Cannot open device '%s': %d: %m", BOTTOM_DEVICE, errno);
        return false;
    }

    if (ioctl(ctx->fd, VIDIOC_S_INPUT, &channel) < 0)
        goto error;

    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.capturemode = 0x8000 /* Preview Mode */;
    if (ioctl(ctx->fd, VIDIOC_S_PARM, &parm) < 0)
        goto error;

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (ioctl(ctx->fd, VIDIOC_S_FMT, &fmt) < 0)
        goto error;

    page_size = getpagesize();
    buffer_size = (fmt.fmt.pix.sizeimage + page_size - 1) & ~(page_size - 1);

    req.count = BUFS_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (ioctl(ctx->fd, VIDIOC_REQBUFS, &req) < 0)
        goto error;

    if (req.count != BUFS_COUNT)
        goto error;

    for (int i = 0; i < BUFS_COUNT; i++) {
        struct v4l2_buffer buf {};

        ctx->bufs_len[i] = buffer_size;
        ctx->bufs[i] = memalign(page_size, buffer_size);

        if (!ctx->bufs[i])
            goto error;

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.index = i;
        buf.m.userptr = (unsigned long)ctx->bufs[i];
        buf.length = ctx->bufs_len[i];

        if (ioctl(ctx->fd, VIDIOC_QBUF, &buf) < 0)
            goto error;
    }

    if (ioctl(ctx->fd, VIDIOC_STREAMON, &type) < 0)
        goto error;

    return true;

error:
    log_error("Error when configuring device ('%s')", BOTTOM_DEVICE);
    _device_deinit(ctx);
    return false;
}


static bool _start_streaming(Context *ctx)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(ctx->fd, VIDIOC_STREAMON, &type) < 0) {
        log_error("Start Bottom streaming failed");
        return false;
    }

    return true;
}

static bool _stop_streaming(Context *ctx)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(ctx->fd, VIDIOC_STREAMOFF, &type) < 0) {
        log_error("Stop Bottom streaming failed");
        return false;
    }

    return true;
}

GstElement *
StreamAeroBottom::create_gstreamer_pipeline(std::map<std::string, std::string> &params) const
{
    GError *error = nullptr;
    std::string pipeline_str;
    GstElement *pipeline;
    GstElement *appsrc;

    Context *ctx = (Context *)calloc(sizeof(Context), 1);
    assert(ctx);

    if (!_device_init(ctx))
        return nullptr;

    if (!_start_streaming(ctx))
        goto device_error;

    /* gstreamer */
    pipeline_str  = create_pipeline(params);
    pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (!pipeline) {
        log_error("Error processing pipeline for RealSense stream device: %s\n",
                  error ? error->message : "unknown error");
        g_clear_error(&error);

        goto error;
    }

    appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "mysource");

    /* setup */
    gst_app_src_set_caps(GST_APP_SRC(appsrc),
                         gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "I420", "width",
                                             G_TYPE_INT, WIDTH, "height", G_TYPE_INT, HEIGHT,
                                             NULL));

    /* setup appsrc */
    g_object_set(G_OBJECT(appsrc), "is-live", TRUE, "format", GST_FORMAT_TIME, NULL);

    /* connect signals */
    GstAppSrcCallbacks cbs;
    cbs.need_data = cb_need_data;
    cbs.enough_data = cb_enough_data;
    cbs.seek_data = cb_seek_data;
    gst_app_src_set_callbacks(GST_APP_SRC_CAST(appsrc), &cbs, ctx, NULL);

    g_object_set_data(G_OBJECT(pipeline), "context", ctx);

    return pipeline;

error:
    _stop_streaming(ctx);
device_error:
    _device_deinit(ctx);
    free(ctx);
    return nullptr;
}

void StreamAeroBottom::finalize_gstreamer_pipeline(GstElement *pipeline)
{
    Context *ctx = (Context *)g_object_get_data(G_OBJECT(pipeline), "context");

    if (!ctx) {
        log_error("Media not created by stream_aero_bottom is being cleared with stream_aero_bottom");
        return;
    }
    _stop_streaming(ctx);
    _device_deinit(ctx);
    free(ctx);
}
