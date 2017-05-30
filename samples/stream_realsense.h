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
#include <string>
#include <vector>

#include "log.h"
#include "stream.h"

class StreamRealSense : public Stream {
public:
    StreamRealSense(const char *path, const char *name, int camera);
    ~StreamRealSense() {}

    const std::string get_path() const override;
    const std::string get_name() const override;
    const std::vector<PixelFormat> &get_formats() const override;
    GstElement *create_gstreamer_pipeline(std::map<std::string, std::string> &params) const override;
    void finalize_gstreamer_pipeline(GstElement *pipeline) override;

private:
    const char *_path;
    const char *_name;
    int _camera;
};
