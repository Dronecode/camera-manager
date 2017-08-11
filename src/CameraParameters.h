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
#include <map>
#include <string>
#include <vector>

#include "log.h"

class CameraParameters {
public:
    CameraParameters();
    virtual ~CameraParameters();
    int getParameterType(std::string param);
    int getParameterID(std::string param);
    int setParameterSupported(std::string key, std::string value);
    int setParameter(std::string param, std::string value);
    std::string getParameter(std::string param);
    const std::map<std::string, std::string> &getParameterList() const { return paramValue; }

    enum param_type {
        PARAM_TYPE_UINT8 = 1,
        PARAM_TYPE_INT8,
        PARAM_TYPE_UINT16,
        PARAM_TYPE_INT16,
        PARAM_TYPE_UINT32,
        PARAM_TYPE_INT32,
        PARAM_TYPE_UINT64,
        PARAM_TYPE_INT64,
        PARAM_TYPE_REAL32,
        PARAM_TYPE_REAL64
    };

    // List of the params
    static const char CAMERA_MODE[];
    static const char BRIGHTNESS[];
    static const char CONTRAST[];
    static const char SATURATION[];
    static const char HUE[];
    static const char WHITE_BALANCE_MODE[];
    static const char GAMMA[];
    static const char GAIN[];
    static const char POWER_LINE_FREQ_MODE[];
    static const char WHITE_BALANCE_TEMPERATURE[];
    static const char SHARPNESS[];
    static const char BACKLIGHT_COMPENSATION[];
    static const char EXPOSURE_MODE[];
    static const char EXPOSURE_ABSOLUTE[];
    static const char IMAGE_SIZE[];
    static const char IMAGE_FORMAT[];
    static const char PIXEL_FORMAT[];
    static const char SCENE_MODE[];
    static const char VIDEO_SIZE[];
    static const char VIDEO_FRAME_FORMAT[];
    static const char VIDEO_SNAPSHOT_SUPPORTED[];

    // Values for camera modes
    static const char CAMERA_MODE_STILL[];
    static const char CAMERA_MODE_VIDEO[];
    static const char CAMERA_MODE_PREVIEW[];
    // Values for white balance settings.
    static const char WHITE_BALANCE_AUTO[];
    static const char WHITE_BALANCE_INCANDESCENT[];
    static const char WHITE_BALANCE_FLUORESCENT[];
    static const char WHITE_BALANCE_WARM_FLUORESCENT[];
    static const char WHITE_BALANCE_DAYLIGHT[];
    static const char WHITE_BALANCE_CLOUDY_DAYLIGHT[];
    static const char WHITE_BALANCE_TWILIGHT[];
    static const char WHITE_BALANCE_SHADE[];
    // Values for Pixel color formats
    static const char PIXEL_FORMAT_YUV422SP[];
    static const char PIXEL_FORMAT_YUV420SP[];
    static const char PIXEL_FORMAT_YUV422I[];
    static const char PIXEL_FORMAT_YUV420P[];
    static const char PIXEL_FORMAT_RGB565[];
    static const char PIXEL_FORMAT_RGBA8888[];
    // Values for Image formats
    static const char IMAGE_FORMAT_JPEG[];
    static const char IMAGE_FORMAT_PNG[];

    enum param_id {
        PARAM_ID_CAMERA_MODE = 1,
        PARAM_ID_BRIGHTNESS,
        PARAM_ID_CONTRAST,
        PARAM_ID_SATURATION,
        PARAM_ID_HUE,
        PARAM_ID_WHITE_BALANCE_MODE,
        PARAM_ID_GAMMA,
        PARAM_ID_GAIN,
        PARAM_ID_POWER_LINE_FREQ_MODE,
        PARAM_ID_WHITE_BALANCE_TEMPERATURE,
        PARAM_ID_SHARPNESS,
        PARAM_ID_BACKLIGHT_COMPENSATION,
        PARAM_ID_EXPOSURE_MODE,
        PARAM_ID_EXPOSURE_ABSOLUTE,
        PARAM_ID_IMAGE_SIZE,
        PARAM_ID_IMAGE_FORMAT,
        PARAM_ID_PIXEL_FORMAT,
        PARAM_ID_SCENE_MODE,
        PARAM_ID_VIDEO_SIZE,
        PARAM_ID_VIDEO_FRAME_FORMAT,
        PARAM_ID_VIDEO_SNAPSHOT_SUPPORTED
    };

    enum id_image_size {
        ID_IMAGE_SIZE_3264x2448 = 0,
        ID_IMAGE_SIZE_3264x1836,
        ID_IMAGE_SIZE_1920x1080
    };

    enum id_video_size {
        ID_VIDEO_SIZE_1920x1080x30 = 1,
        ID_VIDEO_SIZE_1920x1080x15,
        ID_VIDEO_SIZE_1280x720x30,
        ID_VIDEO_SIZE_1280x720x15,
        ID_VIDEO_SIZE_960x540x30,
        ID_VIDEO_SIZE_960x540x15,
        ID_VIDEO_SIZE_848x480x30,
        ID_VIDEO_SIZE_848x480x15,
        ID_VIDEO_SIZE_640x480x60,
        ID_VIDEO_SIZE_640x480x30,
        ID_VIDEO_SIZE_640x480x15,
        ID_VIDEO_SIZE_640x360x30,
        ID_VIDEO_SIZE_640x360x15,
        ID_VIDEO_SIZE_424x240x30,
        ID_VIDEO_SIZE_424x240x15,
        ID_VIDEO_SIZE_320x240x60,
        ID_VIDEO_SIZE_320x240x30,
        ID_VIDEO_SIZE_320x240x15,
        ID_VIDEO_SIZE_320x180x30,
        ID_VIDEO_SIZE_320x180x60
    };

    enum id_camera_mode { ID_CAMERA_MODE_STILL = 0, ID_CAMERA_MODE_VIDEO, ID_CAMERA_MODE_PREVIEW };

    enum id_image_format { ID_IMAGE_FORMAT_JPEG = 0, ID_IMAGE_FORMAT_PNG };

    enum id_white_balance_mode {
        ID_WHITE_BALANCE_MANUAL = 0,
        ID_WHITE_BALANCE_AUTO,
        ID_WHITE_BALANCE_INCANDESCENT,
        ID_WHITE_BALANCE_FLUORESCENT,
        ID_WHITE_BALANCE_WARM_FLUORESCENT,
        ID_WHITE_BALANCE_DAYLIGHT,
        ID_WHITE_BALANCE_CLOUDY_DAYLIGHT,
        ID_WHITE_BALANCE_TWILIGHT,
        ID_WHITE_BALANCE_SHADE
    };

    enum id_exposure_mode { ID_EXPOSURE_MANUAL = 0, ID_EXPOSURE_AUTO };

    enum id_pixel_format {
        ID_PIXEL_FORMAT_YUV422SP = 0,
        ID_PIXEL_FORMAT_YUV420SP,
        ID_PIXEL_FORMAT_YUV422I,
        ID_PIXEL_FORMAT_YUV420P,
        ID_PIXEL_FORMAT_RGB565,
        ID_PIXEL_FORMAT_RGBA8888
    };

    // TODO :: Make exhaustive list of parameters and its possible values
private:
    void initParamIdType();
    void set(std::map<std::string, std::string> &pMap, std::string key, std::string value);
    void set(std::map<std::string, std::pair<int, int>> &pMap, std::string key,
             std::pair<int, int> value);
    std::string get(std::map<std::string, std::string> &pMap, std::string key);
    std::map<std::string, std::string> paramValue;
    std::map<std::string, std::string> paramValuesSupported;
    std::map<std::string, std::pair<int, int>> paramIdType; /*Key->ID,Type*/
};
