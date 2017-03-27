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
#include <map>
#include <string>
#include <vector>

#include "gstreamer_pipeline_builder.h"
#include "log.h"
#include "stream.h"

class StreamV4l2 : public Stream {
public:
    StreamV4l2(GstreamerPipelineBuilder &_gst_builder, std::string device_path,
               std::string device_name);
    ~StreamV4l2() {}

    const std::string get_path() const override;
    const std::string get_name() const override;
    const std::vector<PixelFormat> &get_formats() const override;
    GstElement *create_gstreamer_pipeline(std::map<std::string, std::string> &params) const override;

private:
    std::string name;
    std::string path;
    std::string device_path;
    std::vector<PixelFormat> formats;
    void get_v4l2_info();
    GstreamerPipelineBuilder &gst_builder;
};
