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
#pragma once
#include <string>
#include <vector>

#define V4L2_DEVICE_PATH "/dev/"
#define V4L2_VIDEO_PREFIX "video"

int v4l2_ioctl(int fd, int request, void *arg);
int v4l2_list_devices(std::vector<std::string> &devList);
int v4l2_open(std::string deviceID);
// int v4l2_open(const char *devicepath);
int v4l2_close(int fd);
int v4l2_query_cap(int fd, struct v4l2_capability &vcap);
int v4l2_query_control(int fd);
int v4l2_query_framesizes(int fd);
int v4l2_set_input(int fd, int id);
int v4l2_get_input(int fd);
int v4l2_set_capturemode(int fd, uint32_t mode);
int v4l2_set_pixformat(int fd, uint32_t w, uint32_t h, uint32_t pf);
int v4l2_streamon(int fd);
int v4l2_streamoff(int fd);
int v4l2_buf_req(int fd, uint32_t count);
int v4l2_buf_q(int fd, struct v4l2_buffer *pbuf);
int v4l2_buf_q(int fd, uint32_t i, unsigned long bufptr, uint32_t buflen);
int v4l2_buf_dq(int fd, struct v4l2_buffer *pbuf);
int v4l2_buf_dq(int fd);
int v4l2_get_control(int fd, int ctrl_id);
int v4l2_set_control(int fd, int ctrl_id, int value);
