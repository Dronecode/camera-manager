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
#include <fcntl.h>
#include <gst/app/gstappsrc.h>
#include <librealsense/rs.h>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>

#include "log.h"
#include "samples/stream_realsense.h"

#define WIDTH (640)
#define HEIGHT (480)
#define SIZE (WIDTH * HEIGHT * 3)
#define ONE_METER (999)

struct rgb {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

static void cb_need_data(GstAppSrc *appsrc, guint unused_size, gpointer user_data)
{
    GstFlowReturn ret;
    rs_device *dev = (rs_device *)user_data;

    rs_wait_for_frames(dev, NULL);
    uint16_t *depth = (uint16_t *)rs_get_frame_data(dev, RS_STREAM_DEPTH, NULL);
    struct rgb *rgb = (struct rgb *)rs_get_frame_data(dev, RS_STREAM_COLOR, NULL);
    if (!rgb) {
        log_error("Realsense error");
        return;
    }

    GstBuffer *buffer
        = gst_buffer_new_wrapped_full((GstMemoryFlags)0, rgb, SIZE, 0, SIZE, NULL, NULL);
    for (int i = 0, end = WIDTH * HEIGHT; i < end; ++i) {
        rgb[i].r = depth[i] * 60 / ONE_METER;
        rgb[i].g /= 4;
        rgb[i].b /= 4;
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

StreamRealSense::StreamRealSense()
    : Stream()
{
}

const std::string StreamRealSense::get_path() const
{
    return "/RealSense";
}

const std::string StreamRealSense::get_name() const
{
    return "RealSense Sample Stream";
}

const std::vector<Stream::PixelFormat> &StreamRealSense::get_formats() const
{
    static std::vector<Stream::PixelFormat> formats;
    return formats;
}

GstElement *
StreamRealSense::get_gstreamer_pipeline(std::map<std::string, std::string> &params) const
{
    /* librealsense */
    rs_error *e = 0;
    rs_context *ctx = rs_create_context(RS_API_VERSION, &e);
    if (e) {
        log_error("rs_error was raised when calling %s(%s): %s", rs_get_failed_function(e),
                  rs_get_failed_args(e), rs_get_error_message(e));
        log_error("Current librealsense api version %d", rs_get_api_version(NULL));
        log_error("Compiled for librealsense api version %d", RS_API_VERSION);
        return nullptr;
    }
    rs_device *dev = rs_get_device(ctx, 0, NULL);
    if (!dev) {
        log_error("Unable to access realsense device");
        return nullptr;
    }

    /* Configure all streams to run at VGA resolution at 60 frames per second */
    rs_enable_stream(dev, RS_STREAM_DEPTH, WIDTH, HEIGHT, RS_FORMAT_Z16, 60, NULL);
    rs_enable_stream(dev, RS_STREAM_COLOR, WIDTH, HEIGHT, RS_FORMAT_RGB8, 60, NULL);
    rs_start_device(dev, NULL);

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
    gst_app_src_set_callbacks(GST_APP_SRC_CAST(appsrc), &cbs, dev, NULL);

    g_object_set_data(G_OBJECT(pipeline), "rs_context", ctx);

    return pipeline;
}

void StreamRealSense::finalize_gstreamer_pipeline(GstElement *pipeline)
{
    rs_error *e = 0;
    rs_context *ctx = (rs_context *)g_object_get_data(G_OBJECT(pipeline), "rs_context");

    if (!ctx) {
        log_error("Media not created by stream_realsense is being cleared with stream_realsense");
        return;
    }
    rs_delete_context(ctx, &e);
}
