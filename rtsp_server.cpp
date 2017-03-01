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
#include "rtsp_server.h"
#include "log.h"

#include <string.h>

#define MAX_PIPELINE 200

#define STREAM_TYPE_RTSP_MEDIA_FACTORY stream_rtsp_media_factory_get_type()
G_DECLARE_FINAL_TYPE(StreamRTSPMediaFactory, stream_rtsp_media_factory, STREAM, RTSP_MEDIA_FACTORY,
                     GstRTSPMediaFactory)
struct _StreamRTSPMediaFactory {
    GstRTSPMediaFactory parent;
    RTSPServer *rtsp_server;
};

G_DEFINE_TYPE(StreamRTSPMediaFactory, stream_rtsp_media_factory, GST_TYPE_RTSP_MEDIA_FACTORY)

GstElement *stream_create_element(GstRTSPMediaFactory *factory, const GstRTSPUrl *url)
{
    return ((StreamRTSPMediaFactory *)factory)->rtsp_server->create_element_from_url(url);
}

static void stream_rtsp_media_factory_class_init(StreamRTSPMediaFactoryClass *klass)
{
    GstRTSPMediaFactoryClass *factory = (GstRTSPMediaFactoryClass *)(klass);
    factory->create_element = stream_create_element;
}

static void stream_rtsp_media_factory_init(StreamRTSPMediaFactory *self)
{
}

static GstRTSPMediaFactory *create_media_factory(RTSPServer *server)
{
    StreamRTSPMediaFactory *factory
        = (StreamRTSPMediaFactory *)g_object_new(STREAM_TYPE_RTSP_MEDIA_FACTORY, nullptr);
    factory->rtsp_server = server;
    return (GstRTSPMediaFactory *)factory;
}

RTSPServer::RTSPServer(std::vector<Stream> &_streams, int _port)
    : streams(_streams)
    , is_running(false)
    , port(_port)
{
    gst_init(nullptr, nullptr);
}

RTSPServer::~RTSPServer()
{
    stop();
}

void RTSPServer::start()
{
    gint server_handle;

    if (is_running)
        return;
    is_running = true;

    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;

    server = gst_rtsp_server_new();
    g_object_set(server, "service", "8554", nullptr);

    mounts = gst_rtsp_server_get_mount_points(server);
    factory = create_media_factory(this);

    for (auto s : streams) {
        std::string path = "/" + s.name;
        gst_rtsp_mount_points_add_factory(mounts, path.c_str(), factory);
    }

    g_object_unref(mounts);
    server_handle = gst_rtsp_server_attach(server, nullptr);
    if (!server_handle) {
        log_error("Unable to start rtsp server");
        stop();
    }
}

void RTSPServer::stop()
{
    if (!is_running)
        return;
    is_running = false;

    g_object_unref(server);

    server = nullptr;
}

bool RTSPServer::create_pipeline(char *pipeline, int size, const GstRTSPUrl *url)
{
    if (strlen(url->abspath) <= 1 || url->abspath[0] != '/')
        return false;

    for (auto s : streams) {
        log_debug("%s", s.name.c_str());
        if (s.name == url->abspath + 1) {
            int ret = snprintf(pipeline, MAX_PIPELINE, "( v4l2src device=/dev/%s ! video/x-raw ! "
                     "videoconvert ! jpegenc ! rtpjpegpay name=pay0 )", s.name.c_str());
            if (ret >= MAX_PIPELINE) {
                log_error("Creating pipeline failed. Pipeline size should be lower than %d", MAX_PIPELINE);
                return false;
            }
            log_debug("created pipeline: %s", pipeline);
            return true;
        }
    }

    return false;
}

GstElement *RTSPServer::create_element_from_url(const GstRTSPUrl *url)
{
    char pipeline[MAX_PIPELINE];
    GstElement *element;
    GError *error = nullptr;

    log_debug("RTSP request: %s (query: %s)", url->abspath, url->query);

    if (!create_pipeline(pipeline, MAX_PIPELINE, url)) {
        log_error("No gstreamer pipeline available for request (device: %s - query: %s)",
                  url->abspath, url->query);
        return nullptr;
    }

    element = gst_parse_launch(pipeline, &error);
    if (!element) {
        log_error("Error processing pipeline: %s\n", error ? error->message : "unknown error");
        log_debug("Pipeline %s", pipeline);
        log_error("Request: (device: %s - query: %s)", url->abspath, url->query);
        if (error)
            g_clear_error(&error);

        return nullptr;
    }

    return element;
}
