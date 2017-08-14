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
            mavlink_server.addCameraComponent(*it);
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

static void _insert(std::set<std::string> *value_set, const char *val, size_t val_len)
{
    while (isspace(*val) && val_len > 0) {
        val++;
        val_len--;
    }

    while (val_len > 0 && isspace(val[val_len - 1]))
        val_len--;

    if (val_len)
        value_set->insert(std::string(val, val_len));
}

static int parse_stl_set(const char *val, size_t val_len, void *storage, size_t storage_len)
{
    assert(val);
    assert(val_len);
    assert(storage);

    std::set<std::string> *value_set = (std::set<std::string> *)storage;
    char *end;

    while ((end = (char *)memchr((void *)val, ',', val_len))) {
        if (end - val)
            _insert(value_set, val, end - val);
        val_len = val_len - (end - val) - 1;
        val = end + 1;
    }
    if (val_len)
        _insert(value_set, val, val_len);

    return 0;
}

static int parse_uri(ConfFile &conf, const char *key, char *value)
{
    struct options {
        char *uri_addr;
    } opt;

    static const ConfFile::OptionsTable option_table[] = {
        {key, false, ConfFile::parse_str_dup, OPTIONS_TABLE_STRUCT_FIELD(options, uri_addr)},
    };
    conf.extract_options("uri", option_table, ARRAY_SIZE(option_table), (void *)&opt);
    if (!opt.uri_addr) {
        log_error("URI of camera definition file for %s not found", key);
        return 0;
    }

    log_debug("%s URI : %s", key, opt.uri_addr);
    strcpy(value, opt.uri_addr);
    free(opt.uri_addr);
    return 1;
}

int CameraServer::detect_v4l2devices(ConfFile &conf, std::vector<CameraComponent *> &camList)
{
    int count = 0;
    char uri[140];
    DIR *dp;
    struct dirent *ep;

    std::set<std::string> blacklist;
    static const ConfFile::OptionsTable option_table[] = {
        {"blacklist", false, parse_stl_set, 0, 0},
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
            // TODO::Get the URI for the specific camera device from conf file
            if (parse_uri(conf, ep->d_name, uri))
                camList.push_back(new CameraComponent_V4L2(dev_path, uri));
            else
                camList.push_back(new CameraComponent_V4L2(dev_path));
            count++;
        }
    }

    closedir(dp);
    return count;
}
