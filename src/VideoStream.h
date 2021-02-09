/*
 * This file is part of the Dronecode Camera Manager
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
/**
 *  The VideoStreamInfo structure is used to hold the Streaming information.
 */
struct VideoStreamInfo{
  uint8_t stream_id;		/**Video Stream ID (1 for first, 2 for second, etc.)*/
  uint8_t count	;		/**Number of streams available.*/
  uint8_t type	;		/**VIDEO_STREAM_TYPE	Type of stream.*/
  uint16_t flags;		/**VIDEO_STREAM_STATUS_FLAGS	Bitmap of stream status flags.*/
  float framerate;		/**Hz		Frame rate.*/
  uint16_t resolution_h;	/**pix		Horizontal resolution.*/
  uint16_t resolution_v	;	/**pix		Vertical resolution.*/
  uint32_t bitrate	;	/**bits/s		Bit rate.*/
  uint16_t rotation	;	/**deg		Video image rotation clockwise.*/
  uint16_t hfov	;		/**deg		Horizontal Field of view.*/
  char name [32]	;	/**Stream name.*/
  char uri [160]	;	/**Video stream URI (TCP or RTSP URI ground station should connect to) or port number (UDP port ground station should listen to).*/
};

class VideoStream {
public:
    VideoStream() {}
    ~VideoStream() {}

    enum State { STATE_ERROR = -1, STATE_IDLE = 0, STATE_INIT = 1, STATE_RUN = 2 };

    virtual int init() = 0;
    virtual int uninit() = 0;
    virtual int getInfo(VideoStreamInfo &vidStreamInfo) = 0;
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
    virtual int setTextOverlay(std::string text, int timeSec) { return -1; };
    virtual std::string getTextOverlay() { return {}; };
};
