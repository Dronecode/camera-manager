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
#include <assert.h>
#include <sstream>
#include <stddef.h>

#include "conf_file.h"
#include "gstreamer_pipeline_builder.h"
#include "log.h"
#include "util.h"

GstreamerPipelineBuilder::GstreamerPipelineBuilder()
{
}

static int parse_stl_string(const char *val, size_t val_len, void *storage, size_t storage_len)
{
    assert(val);
    assert(val_len);
    assert(storage);
    assert(storage_len == sizeof(std::string));

    std::string *str = (std::string *)storage;

    *str = std::string(val, val_len);

    return 0;
}

void GstreamerPipelineBuilder::apply_configs(const ConfFile &conf)
{
    static const ConfFile::OptionsTable option_table[] = {
        {"encoder", false, parse_stl_string, OPTIONS_TABLE_STRUCT_FIELD(options, encoder)},
        {"converter", false, parse_stl_string, OPTIONS_TABLE_STRUCT_FIELD(options, converter)},
        {"muxer", false, parse_stl_string, OPTIONS_TABLE_STRUCT_FIELD(options, muxer)},
    };
    conf.extract_options("gstreamer", option_table, ARRAY_SIZE(option_table), (void *)&opt);
}

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
    if (opt.muxer.size() > 0)
        return opt.muxer;
    return "rtph264pay";
}

std::string GstreamerPipelineBuilder::create_converter(std::map<std::string, std::string> &params)
{
    // TODO: support different converters for different formats
    if (opt.converter.size() > 0)
        return opt.converter;
    return "autovideoconvert";
}

std::string GstreamerPipelineBuilder::create_encoder(std::map<std::string, std::string> &params)
{
    // TODO: support different encoders for different formats
    if (opt.encoder.size() > 0)
        return opt.encoder;
    return "x264enc";
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

GstreamerPipelineBuilder &GstreamerPipelineBuilder::get_instance()
{
    static GstreamerPipelineBuilder gst;

    return gst;
}
