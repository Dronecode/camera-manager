/*
 * This file is part of the Camera Streaming Daemon project
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

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "log.h"
#include "v4l2_interface.h"

int v4l2_ioctl(int fd, int request, void *arg)
{
    int r;

    do
        r = ioctl(fd, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

int v4l2_list_devices(std::vector<std::string> &devList)
{
    DIR *dp;
    struct dirent *ep;

    dp = opendir(V4L2_DEVICE_PATH);
    if (dp == NULL) {
        log_error("Could not open directory %d", errno);
        return -1;
    }

    while ((ep = readdir(dp))) {
        if (std::strncmp(V4L2_VIDEO_PREFIX, ep->d_name, sizeof(V4L2_VIDEO_PREFIX) - 1) == 0) {
            log_debug("Found V4L2 camera device %s", ep->d_name);
            // add device path to list
            devList.push_back(std::string(V4L2_DEVICE_PATH) + ep->d_name);
        }
    }

    closedir(dp);
    return 0;
}

int v4l2_open(const char *devicepath)
{
    int fd = -1;
    fd = open(devicepath, O_RDWR, 0);
    if (fd < 0) {
        log_error("Cannot open device '%s': %d: ", devicepath, errno);
    }

    return fd;
}

int v4l2_close(int fd)
{
    if (fd < 1)
        return -1;

    close(fd);
    return 0;
}

int v4l2_query_cap(int fd)
{
    int ret = -1;
    if (fd < 1)
        return ret;

    struct v4l2_capability vcap;
    ret = v4l2_ioctl(fd, VIDIOC_QUERYCAP, &vcap);
    if (ret)
        return ret;

    log_debug("\tDriver name   : %s", vcap.driver);
    log_debug("\tCard type     : %s", vcap.card);
    log_debug("\tBus info      : %s", vcap.bus_info);
    log_debug("\tDriver version: %d.%d.%d", vcap.version >> 16, (vcap.version >> 8) & 0xff,
              vcap.version & 0xff);
    log_debug("\tCapabilities  : 0x%08X", vcap.capabilities);
    if (vcap.capabilities & V4L2_CAP_DEVICE_CAPS) {
        log_debug("\tDevice Caps   : 0x%08X", vcap.device_caps);
    }
    // TODO:: Return the caps
    return 0;
}

int v4l2_query_control(int fd)
{
    int ret = -1;
    if (fd < 1)
        return ret;

    const unsigned next_fl = V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;
    struct v4l2_control ctrl = {0};
    struct v4l2_queryctrl qctrl = {0};
    qctrl.id = next_fl;
    while (v4l2_ioctl(fd, VIDIOC_QUERYCTRL, &qctrl) == 0) {
        if ((qctrl.flags & V4L2_CTRL_TYPE_BOOLEAN) || (qctrl.type & V4L2_CTRL_TYPE_INTEGER)) {
            log_debug("Ctrl: %s Min:%d Max:%d Step:%d dflt:%d", qctrl.name, qctrl.minimum,
                      qctrl.maximum, qctrl.step, qctrl.default_value);
            ctrl.id = qctrl.id;
            if (v4l2_ioctl(fd, VIDIOC_G_CTRL, &ctrl) == 0) {
                log_debug("Ctrl: %s Id:%x Value:%d\n", qctrl.name, ctrl.id, ctrl.value);
            }
        }
        qctrl.id |= next_fl;
    }
    // TODO:: Return the list of controls
    return 0;
}

int v4l2_query_framesizes(int fd)
{
    int ret = -1;
    if (fd < 1)
        return ret;

    struct v4l2_fmtdesc fmt = {};
    struct v4l2_frmsizeenum frame_size = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.index = 0;
    while (v4l2_ioctl(fd, VIDIOC_ENUM_FMT, &fmt) != -1) {
        // TODO:: Save the format in some DS
        log_debug("Pixel Format: %u", fmt.pixelformat);
        log_debug("Name : %s\n", fmt.description);
        frame_size.index = 0;
        frame_size.pixel_format = fmt.pixelformat;
        while (v4l2_ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frame_size) != -1) {
            switch (frame_size.type) {
            case V4L2_FRMSIZE_TYPE_DISCRETE:
                log_debug(" FrameSize: %ux%u\n", frame_size.discrete.width,
                          frame_size.discrete.height);
                break;
            case V4L2_FRMSIZE_TYPE_CONTINUOUS:
            case V4L2_FRMSIZE_TYPE_STEPWISE:
                log_debug(" FrameSize: %ux%u -> %ux%u\n", frame_size.stepwise.min_width,
                          frame_size.stepwise.min_height, frame_size.stepwise.max_width,
                          frame_size.stepwise.max_height);
                break;
            }
            // TODO::Save the framesizes against format in some DS
        }

        fmt.index++;
    }

    // TODO::Return the list of pixformats & framesizes
    return 0;
}

int v4l2_get_control(int fd, int ctrl_id)
{
    int ret = -1;
    if (fd < 1)
        return ret;

    struct v4l2_control ctrl = {0};
    ctrl.id = ctrl_id;
    ret = v4l2_ioctl(fd, VIDIOC_G_CTRL, &ctrl);
    if (ret == 0)
        return ctrl.value;
    else
        return ret;
}

int v4l2_set_control(int fd, int ctrl_id, int value)
{
    int ret = -1;
    if (fd < 1)
        return ret;

    struct v4l2_control c = {0};
    c.id = ctrl_id;
    c.value = value;
    ret = v4l2_ioctl(fd, VIDIOC_S_CTRL, &c);

    return ret;
}
