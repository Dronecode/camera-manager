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

#include <assert.h>
#include <cstring>
#include <set>
#include <sstream>
#include <string.h>

#include "CameraComponent_V4L2.h"
#include "CameraServer.h"
#include "log.h"
#include "mavlink_server.h"
#include "util.h"

#define DEVICE_PATH "/dev/"
#define VIDEO_PREFIX "video"
#define DEFAULT_SERVICE_PORT 8554

CameraServer::CameraServer(ConfFile &conf)
    : mavlink_server(conf, streams, rtsp_server)
    , rtsp_server(streams, DEFAULT_SERVICE_PORT)
    , cameraCount(0)
{
    cameraCount = detectCamera(conf);

    std::vector<CameraComponent *>::iterator it;
    for (it = cameraList.begin(); it != cameraList.end(); it++) {
        if (*it) {
            if (mavlink_server.addCameraComponent(*it) == -1)
                log_error("Error in adding Camera Component");
        }
    }
}

CameraServer::~CameraServer()
{
    stop();
}

void CameraServer::start()
{
    log_error("CAMERA SERVER START");

    mavlink_server.start();
}

void CameraServer::stop()
{
    mavlink_server.stop();
}

// prepare the list of cameras in the system
int CameraServer::detectCamera(ConfFile &conf)
{
    int count = 0;

    count += detect_v4l2devices(conf, cameraList);

    return count;
}

int CameraServer::detect_v4l2devices(ConfFile &conf, std::vector<CameraComponent *> &camList)
{
    int count = 0;
    char *uri_addr = 0;
    DIR *dp;
    struct dirent *ep;

    std::set<std::string> blacklist;
    static const ConfFile::OptionsTable option_table[] = {
        {"blacklist", false, ConfFile::parse_stl_set, 0, 0},
    };

    conf.extract_options("v4l2", option_table, 1, (void *)&blacklist);

    dp = opendir(DEVICE_PATH);
    if (dp == NULL) {
        log_error("Could not open directory");
        return 0;
    }

    while ((ep = readdir(dp))) {
        if (std::strncmp(VIDEO_PREFIX, ep->d_name, sizeof(VIDEO_PREFIX) - 1) == 0) {
            std::string dev_path = DEVICE_PATH;
            // Don't add stream if it is in a blacklist
            if (blacklist.find(ep->d_name) != blacklist.end())
                continue;
            log_debug("Found V4L2 camera device %s", ep->d_name);
            dev_path.append(ep->d_name);
            if (!conf.extract_options("uri", ep->d_name, &uri_addr)) {
                std::string uriString(uri_addr);
                camList.push_back(new CameraComponent_V4L2(dev_path, uriString));
                free(uri_addr);
            } else {
                log_warning("Camera Definition for device:%s not found", ep->d_name);
                camList.push_back(new CameraComponent_V4L2(dev_path));
            }
            count++;
        }
    }

    closedir(dp);
    return count;
}
