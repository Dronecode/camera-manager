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
#include <string>
#include <vector>

#include "log.h"

class Stream {
public:
    struct FrameSize {
    public:
        uint32_t width, height;

    private:
        friend std::ostream &operator<<(std::ostream &os, const FrameSize &fs);
    };

    struct PixelFormat {
    public:
        uint32_t pixel_format;
        std::vector<FrameSize> frame_sizes;

    private:
        friend std::ostream &operator<<(std::ostream &os, const PixelFormat &pf);
    };

public:
    virtual ~Stream(){};
    virtual const std::string get_path() const = 0;
    virtual const std::string get_name() const = 0;
    virtual GstElement *get_gstreamer_pipeline() const { return nullptr; }
    virtual const std::vector<PixelFormat> &get_formats() const = 0;
};
