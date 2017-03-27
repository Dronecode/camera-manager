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
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>

#include "log.h"
#include "samples/stream_custom.h"

StreamCustom::StreamCustom()
    : Stream()
{
}

const std::string StreamCustom::get_path() const
{
    return "/custom";
}

const std::string StreamCustom::get_name() const
{
    return "Custom Stream";
}

const std::vector<Stream::PixelFormat> &StreamCustom::get_formats() const
{
    static std::vector<Stream::PixelFormat> formats;
    return formats;
}

GstElement *StreamCustom::create_gstreamer_pipeline(std::map<std::string, std::string> &params) const
{
    GError *error = nullptr;
    GstElement *pipeline;

    pipeline = gst_parse_launch("videotestsrc ! video/x-raw,width=640,height=480 ! "
                                "videoconvert ! jpegenc ! rtpjpegpay name=pay0",
                                &error);
    if (!pipeline) {
        log_error("Error processing pipeline for custom stream device: %s\n",
                  error ? error->message : "unknown error");
        if (error)
            g_clear_error(&error);

        return nullptr;
    }

    return pipeline;
}
