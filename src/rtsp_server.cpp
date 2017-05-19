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
#include <sstream>
#include <string.h>

#include "rtsp_server.h"
#include "log.h"

static void (*media_dispose)(GObject *obj) = nullptr;
static GstRTSPMedia *(*factory_construct)(GstRTSPMediaFactory *factory, const GstRTSPUrl *url)
    = nullptr;

static void stream_media_dispose(GObject *obj)
{
    Stream *stream = (Stream *)g_object_get_data(obj, "stream");
    GObject *element = nullptr;
    g_object_get(obj, "element", &element, NULL);

    if (stream && element)
        stream->finalize_gstreamer_pipeline(GST_ELEMENT(element));

    if (media_dispose)
        media_dispose(obj);
    stream->is_streaming = false;
}

GstElement *stream_create_element(GstRTSPMediaFactory *factory, const GstRTSPUrl *url)
{
    RTSPServer *rtsp_server = (RTSPServer *)g_object_get_data(G_OBJECT(factory), "rtsp_server");
    if (!rtsp_server) {
        log_error("create_element called in invalid media factory.");
        return nullptr;
    }
    return rtsp_server->create_element_from_url(url);
}

GstRTSPMedia *stream_construct(GstRTSPMediaFactory *factory, const GstRTSPUrl *url)
{
    if (!factory_construct)
        return nullptr;

    GstRTSPMedia *media = factory_construct(factory, url);
    if (!media)
        return nullptr;

    RTSPServer *rtsp_server = (RTSPServer *)g_object_get_data(G_OBJECT(factory), "rtsp_server");
    if (!rtsp_server)
        return nullptr;

    Stream *stream = rtsp_server->find_stream_by_path(url->abspath);
    g_object_set_data(G_OBJECT(media), "stream", stream);

    if (!media_dispose) {
        GstRTSPMediaClass *media_class = GST_RTSP_MEDIA_GET_CLASS(media);
        media_dispose = G_OBJECT_CLASS(media_class)->dispose;
        G_OBJECT_CLASS(media_class)->dispose = stream_media_dispose;
    }
    return media;
}

static GstRTSPMediaFactory *create_media_factory(RTSPServer *rtsp_server)
{
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();
    if (!factory)
        return nullptr;

    g_object_set_data(G_OBJECT(factory), "rtsp_server", rtsp_server);
    if (!factory_construct) {
        GstRTSPMediaFactoryClass *factory_class = GST_RTSP_MEDIA_FACTORY_GET_CLASS(factory);
        factory_class->create_element = stream_create_element;
        factory_construct = factory_class->construct;
        factory_class->construct = stream_construct;
    }

    return factory;
}

RTSPServer::RTSPServer(std::vector<std::unique_ptr<Stream>> &_streams, int _port)
    : streams(_streams)
    , is_running(false)
    , port(_port)
    , server(nullptr)
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

    GstRTSPMountPoints *mounts = nullptr;
    GstRTSPMediaFactory *factory = nullptr;

    server = gst_rtsp_server_new();
    g_object_set(server, "service", "8554", nullptr);

    if (streams.size() > 0) {
        mounts = gst_rtsp_server_get_mount_points(server);
        if (!mounts)
            goto error;
        factory = create_media_factory(this);
        if (!factory)
            goto error_factory;

        for (auto &s : streams)
            gst_rtsp_mount_points_add_factory(mounts, s->get_path().c_str(), factory);
        g_object_unref(mounts);
    }

    server_handle = gst_rtsp_server_attach(server, nullptr);
    if (!server_handle)
        goto error;
    return;

error_factory:
    g_object_unref(mounts);
error:
    g_object_unref(server);
    log_error("Unable to start rtsp server");
    is_running = false;
    server = nullptr;
}

void RTSPServer::stop()
{
    if (!is_running)
        return;
    is_running = false;

    g_object_unref(server);

    server = nullptr;
}

Stream *RTSPServer::find_stream_by_path(const char *path)
{
    for (auto &s : streams)
        if (s->get_path() == path)
            return s.get();

    return nullptr;
}

void RTSPServer::append_to_map(std::map<std::string, std::string> &map, const std::string &param)
{
    size_t j = param.find('=');
    if (j == std::string::npos)
        return;

    map[param.substr(0, j)] = param.substr(j + 1);
}

std::map<std::string, std::string> RTSPServer::parse_uri_query(const char *query)
{
    std::map<std::string, std::string> map;

    if (!query || !query[0])
        return map;

    std::string query_str = query;
    size_t i = 0, j;

    j = query_str.find('&');
    while (j != std::string::npos) {
        append_to_map(map, query_str.substr(i, j - i));
        i = j + 1;
        j = query_str.find('&', j + 1);
    }
    append_to_map(map, query_str.substr(i, j - i));

    return map;
}

GstElement *RTSPServer::create_element_from_url(const GstRTSPUrl *url)
{
    GstElement *pipeline;
    Stream *stream;
    std::map<std::string, std::string> params;

    log_debug("RTSP request: %s (query: %s)", url->abspath, url->query);
    stream = find_stream_by_path(url->abspath);
    if (!stream)
        goto error;

    params = parse_uri_query(url->query);
    pipeline = stream->create_gstreamer_pipeline(params);
    if (!pipeline)
        goto error;

    stream->is_streaming = true;
    return pipeline;
error:
    log_error("No gstreamer pipeline available for request (device_path: %s - query: %s)",
              url->abspath, url->query);
    return nullptr;
}

std::string RTSPServer::get_rtsp_uri(const char *ip, Stream &stream)
{
    assert(ip);

    std::stringstream ss;
    ss << "rtsp://" << ip << ":" << port << "/" << stream.get_path();

    return ss.str();
}
