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
#include <sstream>

#include "gstreamer_pipeline_builder.h"
#include "log.h"
#include "settings.h"

std::string GstreamerPipelineBuilder::create_caps(std::map<std::string, std::string> &params)
{
    std::stringstream ss;
    std::string width, height;
    ss << "capsfilter caps=\"video/x-raw";

    width = params["width"];
    height = params["height"];

    if (width != "" && height != "") {
        ss << ", width=";
        ss << width;
        ss << ", height=";
        ss << height;
    }
    ss << "\"";

    return ss.str();
}

std::string GstreamerPipelineBuilder::create_muxer(std::map<std::string, std::string> &params)
{
    // TODO: support different muxers for different formats
    std::string muxer = Settings::get_instance().get_sections()["gstreamer"]["muxer"];
    if (muxer.size() > 0)
        return muxer;
    return "rtpjpegpay";
}

std::string GstreamerPipelineBuilder::create_converter(std::map<std::string, std::string> &params)
{
    // TODO: support different converters for different formats
    std::string converter = Settings::get_instance().get_sections()["gstreamer"]["converter"];
    if (converter.size() > 0)
        return converter;
    return "videoconvert";
}

std::string GstreamerPipelineBuilder::create_encoder(std::map<std::string, std::string> &params)
{
    // TODO: support different encoders for different formats
    std::string encoder = Settings::get_instance().get_sections()["gstreamer"]["encoder"];
    if (encoder.size() > 0)
        return encoder;
    return "jpegenc";
}

std::string GstreamerPipelineBuilder::create_pipeline(std::string source,
                                                      std::map<std::string, std::string> &params)
{
    std::stringstream ss;
    ss << source << " ! " << create_caps(params) + " ! " + create_converter(params) << " ! "
       << create_encoder(params) << " ! " << create_muxer(params) << " name=pay0";
    log_debug("Gstreamer pipeline: %s", ss.str().c_str());
    return ss.str();
}
