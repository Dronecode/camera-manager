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

#include <set>

#include "CameraComponent.h"
#include "CameraServer.h"
#include "log.h"
#include "mavlink_server.h"
#include "v4l2_interface.h"

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

    // Free up all the resources  allocated
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

    count += detect_devices_v4l2(conf, cameraList);

    return count;
}

int CameraServer::detect_devices_v4l2(ConfFile &conf, std::vector<CameraComponent *> &camList)
{
    int count = 0;
    char *uri_addr = 0;

    // Get the list of devices in the system
    std::vector<std::string> v4l2List;
    v4l2_list_devices(v4l2List);
    if (v4l2List.empty())
        return 0;

    // Get the blacklisted devices
    std::set<std::string> blacklist;
    static const ConfFile::OptionsTable option_table[] = {
        {"blacklist", false, ConfFile::parse_stl_set, 0, 0},
    };
    conf.extract_options("v4l2", option_table, 1, (void *)&blacklist);

    // Create camera components for devices not blacklisted
    std::string dev_name;
    for (auto dev_path : v4l2List) {
        log_debug("v4l2 device :: %s", dev_path.c_str());
        dev_name = dev_path.substr(sizeof(V4L2_DEVICE_PATH) - 1);
        if (blacklist.find(dev_name) != blacklist.end())
            continue;
        if (!conf.extract_options("uri", dev_name.c_str(), &uri_addr)) {
            std::string uriString(uri_addr);
            camList.push_back(new CameraComponent(dev_path, uriString));
            free(uri_addr);
        } else {
            log_warning("Camera Definition for device:%s not found", dev_name.c_str());
            camList.push_back(new CameraComponent(dev_path));
        }
        count++;
    }

    return count;
}
