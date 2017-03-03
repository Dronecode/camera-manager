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
#include <map>

class GstreamerPipelineBuilder {
public:
    virtual std::string create_caps(std::map<std::string, std::string> &params);
    virtual std::string create_muxer(std::map<std::string, std::string> &params);
    virtual std::string create_converter(std::map<std::string, std::string> &params);
    virtual std::string create_encoder(std::map<std::string, std::string> &params);
    virtual std::string create_pipeline(std::string source, std::map<std::string, std::string> &params);
private:
};
