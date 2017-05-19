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
#include <linux/videodev2.h>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>

#include "gstreamer_pipeline_builder.h"
#include "log.h"
#include "stream_v4l2.h"

StreamV4l2::StreamV4l2(std::string _path, std::string _device_path)
    : Stream()
    , name("")
    , path(_path)
    , device_path(_device_path)
{
    get_v4l2_info();
}

const std::string StreamV4l2::get_path() const
{
    return path;
}

const std::string StreamV4l2::get_name() const
{
    return name;
}

void StreamV4l2::get_v4l2_info()
{
    int fd = open(device_path.c_str(), O_RDONLY);
    if (fd == -1) {
        log_error("Not able to load StreamV4l2 %s formats.", get_name().c_str());
        return;
    }

    struct v4l2_fmtdesc fmt = {};
    struct v4l2_frmsizeenum frame_size = {};
    struct v4l2_capability cap;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) != -1) {
        Stream::PixelFormat format{fmt.pixelformat};

        frame_size.index = 0;
        frame_size.pixel_format = fmt.pixelformat;
        while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frame_size) != -1) {
            if (frame_size.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                format.frame_sizes.emplace_back(
                    FrameSize{frame_size.discrete.width, frame_size.discrete.height});
                frame_size.index++;
            } else {
                // TODO: Support different frame size types
            }
        }
        formats.push_back(format);
        fmt.index++;
    }

    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) != -1) {
        name = (const char *)cap.card;
    } else {
        name = path.substr(1);
    }
    close(fd);
}

GstElement *StreamV4l2::create_gstreamer_pipeline(std::map<std::string, std::string> &params) const
{
    GError *error = nullptr;
    GstElement *pipeline;
    std::string source = "v4l2src device=";
    source.append(device_path);

    std::string pipeline_str
        = GstreamerPipelineBuilder::get_instance().create_pipeline(source, params);
    pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (!pipeline) {
        log_error("Error processing pipeline for device %s: %s\n", device_path.c_str(),
                  error ? error->message : "unknown error");
        log_debug("Pipeline %s", pipeline_str.c_str());
        if (error)
            g_clear_error(&error);

        return nullptr;
    }

    return pipeline;
}
