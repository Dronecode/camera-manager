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
#pragma once

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <vector>

#include "stream.h"

class RTSPServer {
public:
    RTSPServer(std::vector<Stream> &_streams, int _port);
    ~RTSPServer();
    void start();
    void stop();

private:
    const std::vector<Stream> &streams;
    bool is_running;
    int port;

    gint server_handle;
    GstRTSPServer *server;
    GstElement *create_element_from_url(const GstRTSPUrl *url);
    bool create_pipeline(char *pipeline, int size, const GstRTSPUrl *url);
    friend GstElement *stream_create_element(GstRTSPMediaFactory *factory, const GstRTSPUrl *url);
};
