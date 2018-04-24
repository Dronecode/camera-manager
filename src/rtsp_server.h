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
#pragma once

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <map>
#include <memory>
#include <vector>

#include "stream.h"

class RTSPServer {
public:
    RTSPServer(std::vector<std::unique_ptr<Stream>> &_streams, int _port);
    ~RTSPServer();
    void start();
    void stop();

    std::string get_rtsp_uri(const char *ip, Stream &stream, const char *query);

private:
    const std::vector<std::unique_ptr<Stream>> &streams;
    bool is_running;
    int port;

    GstRTSPServer *server;
    GstElement *create_element_from_url(const GstRTSPUrl *url);
    Stream *find_stream_by_path(const char *path);
    std::map<std::string, std::string> parse_uri_query(const char *query);
    void append_to_map(std::map<std::string, std::string> &map, const std::string &param);
    friend GstElement *stream_create_element(GstRTSPMediaFactory *factory, const GstRTSPUrl *url);
    friend void stream_construct(GObject *obj);
    friend GstRTSPMedia *stream_construct(GstRTSPMediaFactory *factory, const GstRTSPUrl *url);
};
