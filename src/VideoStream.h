/*
 * This file is part of the Camera Streaming Daemon
 *
 * Copyright (C) 2018  Intel Corporation. All rights reserved.
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

class VideoStream {
public:
    VideoStream() {}
    ~VideoStream() {}

    enum State { STATE_ERROR = -1, STATE_IDLE = 0, STATE_INIT = 1, STATE_RUN = 2 };

    virtual int init() = 0;
    virtual int uninit() = 0;
    virtual int start() = 0;
    virtual int stop() = 0;
    virtual int getState() = 0;
    virtual int setResolution(int imgWidth, int imgHeight) = 0;
    virtual int getResolution(int &imgWidth, int &imgHeight) = 0;
    virtual int setFormat(int vidFormat) = 0;
    virtual int getFormat() = 0;
    // The host/IP/Multicast group to send the packets to
    virtual int setAddress(std::string ipAddr) = 0;
    virtual std::string getAddress() = 0;
    // The port to send the packets to
    virtual int setPort(uint32_t port) = 0;
    virtual int getPort() = 0;
    virtual int setTextOverlay(std::string text, int timeSec) = 0;
    virtual std::string getTextOverlay() = 0;
};
