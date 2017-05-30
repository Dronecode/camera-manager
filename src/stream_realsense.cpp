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
#include <fcntl.h>
#include <gst/app/gstappsrc.h>
#include <librealsense/rs.h>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>

#include "log.h"
#include "stream_realsense.h"

#define WIDTH (640)
#define HEIGHT (480)
#define SIZE (WIDTH * HEIGHT * 3)
#define ONE_METER (999)

struct rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct Context {
    rs_device *dev;
    rs_context *rs_ctx;
    int camera;
    struct rgb rgb_data[];
};

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

static void cb_need_data(GstAppSrc *appsrc, guint unused_size, gpointer user_data)
{
    GstFlowReturn ret;
    Context *ctx = (Context *)user_data;

    rs_wait_for_frames(ctx->dev, NULL);
    GstBuffer *buffer
        = gst_buffer_new_wrapped_full((GstMemoryFlags)0, ctx->rgb_data, SIZE, 0, SIZE, NULL, NULL);
    assert(buffer);

    if (ctx->camera == RS_STREAM_INFRARED || ctx->camera == RS_STREAM_INFRARED2) {
        uint8_t *ir = (uint8_t *)rs_get_frame_data(ctx->dev, (rs_stream)ctx->camera, NULL);
        if (!ir) {
            log_error("No infrared data. Not building frame");
            return;
        }

        for (int i = 0, end = WIDTH * HEIGHT; i < end; ++i) {
            ctx->rgb_data[i].r = ir[i];
            ctx->rgb_data[i].g = ir[i];
            ctx->rgb_data[i].b = ir[i];
        }
    } else {
        uint16_t *depth = (uint16_t *)rs_get_frame_data(ctx->dev, RS_STREAM_DEPTH, NULL);
        if (!depth) {
            log_error("No depth data. Not building frame");
            return;
        }

        for (int i = 0, end = WIDTH * HEIGHT; i < end; ++i) {
            uint8_t rainbow[3];
            rainbow_scale((double)depth[i], rainbow);

            ctx->rgb_data[i].r = rainbow[0];
            ctx->rgb_data[i].g = rainbow[1];
            ctx->rgb_data[i].b = rainbow[2];
        }
    }
    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
}

static void cb_enough_data(GstAppSrc *src, gpointer user_data)
{
}

static gboolean cb_seek_data(GstAppSrc *src, guint64 offset, gpointer user_data)
{
    return TRUE;
}

StreamRealSense::StreamRealSense(const char *path, const char *name, int camera)
    : Stream()
    , _path(path)
    , _name(name)
    , _camera(camera)
{
}

const std::string StreamRealSense::get_path() const
{
    return _path;
}

const std::string StreamRealSense::get_name() const
{
    return _name;
}

const std::vector<Stream::PixelFormat> &StreamRealSense::get_formats() const
{
    static std::vector<Stream::PixelFormat> formats;
    return formats;
}

GstElement *
StreamRealSense::create_gstreamer_pipeline(std::map<std::string, std::string> &params) const
{
    Context *ctx = (Context *)malloc(sizeof(Context) + SIZE);
    assert(ctx);

    /* librealsense */
    rs_error *e = 0;
    ctx->rs_ctx = rs_create_context(RS_API_VERSION, &e);
    if (e) {
        log_error("rs_error was raised when calling %s(%s): %s", rs_get_failed_function(e),
                  rs_get_failed_args(e), rs_get_error_message(e));
        log_error("Current librealsense api version %d", rs_get_api_version(NULL));
        log_error("Compiled for librealsense api version %d", RS_API_VERSION);
        return nullptr;
    }
    ctx->dev = rs_get_device(ctx->rs_ctx, 0, NULL);
    if (!ctx->dev) {
        log_error("Unable to access realsense device");
        return nullptr;
    }

    /* Configure all streams to run at VGA resolution at 60 frames per second */
    if (_camera == RS_STREAM_INFRARED || _camera == RS_STREAM_INFRARED2)
        rs_enable_stream(ctx->dev, (rs_stream)_camera, WIDTH, HEIGHT, RS_FORMAT_Y8, 60, NULL);
    else
        rs_enable_stream(ctx->dev, RS_STREAM_DEPTH, WIDTH, HEIGHT, RS_FORMAT_Z16, 60, NULL);
    ctx->camera = _camera;
    rs_start_device(ctx->dev, NULL);

    if (rs_device_supports_option(ctx->dev, RS_OPTION_R200_EMITTER_ENABLED, NULL))
        rs_set_device_option(ctx->dev, RS_OPTION_R200_EMITTER_ENABLED, 1, NULL);
    if (rs_device_supports_option(ctx->dev, RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED, NULL))
        rs_set_device_option(ctx->dev, RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED, 1, NULL);

    /* gstreamer */
    GError *error = nullptr;
    GstElement *pipeline;

    pipeline = gst_parse_launch("appsrc name=mysource ! videoconvert ! "
                                "video/x-raw,width=640,height=480,format=NV12 ! vaapih264enc ! "
                                "rtph264pay name=pay0",
                                &error);
    if (!pipeline) {
        log_error("Error processing pipeline for RealSense stream device: %s\n",
                  error ? error->message : "unknown error");
        g_clear_error(&error);

        return nullptr;
    }

    GstElement *appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "mysource");

    /* setup */
    gst_app_src_set_caps(GST_APP_SRC(appsrc),
                         gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB", "width",
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
}

void StreamRealSense::finalize_gstreamer_pipeline(GstElement *pipeline)
{
    Context *ctx = (Context *)g_object_get_data(G_OBJECT(pipeline), "context");

    if (!ctx) {
        log_error("Media not created by stream_realsense is being cleared with stream_realsense");
        return;
    }
    rs_delete_context(ctx->rs_ctx, NULL);
    free(ctx);
}
