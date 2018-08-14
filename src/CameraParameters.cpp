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
#include <cstring>
#include <sstream>
#include <string.h>

#include "CameraParameters.h"
#include "log.h"

const char CameraParameters::CAMERA_MODE[] = "camera-mode";
const char CameraParameters::BRIGHTNESS[] = "brightness";
const char CameraParameters::CONTRAST[] = "contrast";
const char CameraParameters::SATURATION[] = "saturation";
const char CameraParameters::HUE[] = "hue";
const char CameraParameters::WHITE_BALANCE_MODE[] = "wb-mode";
const char CameraParameters::GAMMA[] = "gamma";
const char CameraParameters::GAIN[] = "gain";
const char CameraParameters::POWER_LINE_FREQ_MODE[] = "power-mode";
const char CameraParameters::WHITE_BALANCE_TEMPERATURE[] = "wb-temp";
const char CameraParameters::SHARPNESS[] = "sharpness";
const char CameraParameters::BACKLIGHT_COMPENSATION[] = "backlight";
const char CameraParameters::EXPOSURE_MODE[] = "exp-mode";
const char CameraParameters::EXPOSURE[] = "exposure";
const char CameraParameters::EXPOSURE_ABSOLUTE[] = "exp-absolute";
const char CameraParameters::EXPOSURE_AUTO_PRIORITY[] = "exp-priority";
const char CameraParameters::IMAGE_SIZE[] = "image-size";
const char CameraParameters::IMAGE_FORMAT[] = "image-format";
const char CameraParameters::PIXEL_FORMAT[] = "pixel-format";
const char CameraParameters::SCENE_MODE[] = "scene-mode";
const char CameraParameters::VIDEO_SIZE[] = "video-size";
const char CameraParameters::VIDEO_FRAME_FORMAT[] = "video-format";
const char CameraParameters::IMAGE_CAPTURE[] = "img-capture";
const char CameraParameters::VIDEO_CAPTURE[] = "vid-capture";
const char CameraParameters::VIDEO_SNAPSHOT[] = "vid-snapshot";
const char CameraParameters::IMAGE_VIDEOSHOT[] = "img-videoshot";

/* ++Need to check if required, as we have defined values as enums++ */

// Values for white balance settings.
const char CameraParameters::WHITE_BALANCE_AUTO[] = "auto";
const char CameraParameters::WHITE_BALANCE_INCANDESCENT[] = "incandescent";
const char CameraParameters::WHITE_BALANCE_FLUORESCENT[] = "fluorescent";
const char CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT[] = "warm-fluorescent";
const char CameraParameters::WHITE_BALANCE_DAYLIGHT[] = "daylight";
const char CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT[] = "cloudy-daylight";
const char CameraParameters::WHITE_BALANCE_TWILIGHT[] = "twilight";
const char CameraParameters::WHITE_BALANCE_SHADE[] = "shade";
/* --Need to check if required-- */

CameraParameters::CameraParameters()
{
    // initParamIdType();
}

CameraParameters::~CameraParameters()
{
}

bool CameraParameters::setParameterValuesSupported(std::string param, std::string value)
{
    if (param.size() > CAM_PARAM_ID_LEN || value.size() > CAM_PARAM_VALUE_LEN)
        return false;

    paramValuesSupported[param] = value;
    return true;
}

bool CameraParameters::setParameterIdType(std::string param, int paramId, int paramType)
{
    // TODO :: Check if the param is not already declared
    // TODO :: Check the paramid doesnt clash with others

    if (param.empty() || paramId < 0 || paramType < 0) {
        log_error("Invalid Argument");
        return false;
    }

    paramIdType[param] = std::make_pair(paramId, paramType);
    return true;
}

int CameraParameters::getParameterID(std::string param)
{
    int ret = -1;
    std::map<std::string, std::pair<int, int>>::iterator it = paramIdType.find(param);
    if (it != paramIdType.end()) {
        ret = it->second.first;
    } else
        ret = -1;
    return ret;
}

int CameraParameters::getParameterType(std::string param)
{
    int ret = -1;
    std::map<std::string, std::pair<int, int>>::iterator it = paramIdType.find(param);
    if (it != paramIdType.end()) {
        ret = it->second.second;
    } else
        ret = -1;
    return ret;
}

bool CameraParameters::setParameter(std::string key, std::string value)
{
    if (key.size() > CAM_PARAM_ID_LEN || value.size() > CAM_PARAM_VALUE_LEN)
        return false;

    paramValue[key] = value;
    return true;
}

bool CameraParameters::setParameter(std::string param_id, float param_value)
{
    cam_param_union_t u;
    u.param_float = param_value;
    std::string str(reinterpret_cast<char const *>(u.bytes), CAM_PARAM_VALUE_LEN);
    return setParameter(param_id, str);
}
bool CameraParameters::setParameter(std::string param_id, uint32_t param_value)
{
    cam_param_union_t u;
    u.param_uint32 = param_value;
    std::string str(reinterpret_cast<char const *>(u.bytes), CAM_PARAM_VALUE_LEN);
    return setParameter(param_id, str);
}
bool CameraParameters::setParameter(std::string param_id, int32_t param_value)
{
    cam_param_union_t u;
    u.param_int32 = param_value;
    std::string str(reinterpret_cast<char const *>(u.bytes), CAM_PARAM_VALUE_LEN);
    return setParameter(param_id, str);
}
bool CameraParameters::setParameter(std::string param_id, uint8_t param_value)
{
    cam_param_union_t u;
    u.param_uint8 = param_value;
    std::string str(reinterpret_cast<char const *>(u.bytes), CAM_PARAM_VALUE_LEN);
    return setParameter(param_id, str);
}

std::string CameraParameters::getParameter(std::string key)
{
    if (paramValue.find(key) == paramValue.end())
        return std::string();
    else
        return paramValue[key];
}

void CameraParameters::initParamIdType()
{
    paramIdType[CAMERA_MODE] = std::make_pair(PARAM_ID_CAMERA_MODE, PARAM_TYPE_UINT32);
    paramIdType[BRIGHTNESS] = std::make_pair(PARAM_ID_BRIGHTNESS, PARAM_TYPE_UINT32);
    paramIdType[CONTRAST] = std::make_pair(PARAM_ID_CONTRAST, PARAM_TYPE_UINT32);
    paramIdType[SATURATION] = std::make_pair(PARAM_ID_SATURATION, PARAM_TYPE_UINT32);
    paramIdType[HUE] = std::make_pair(PARAM_ID_HUE, PARAM_TYPE_INT32);
    paramIdType[WHITE_BALANCE_MODE]
        = std::make_pair(PARAM_ID_WHITE_BALANCE_MODE, PARAM_TYPE_UINT32);
    paramIdType[GAMMA] = std::make_pair(PARAM_ID_GAMMA, PARAM_TYPE_UINT32);
    paramIdType[GAIN] = std::make_pair(PARAM_ID_GAIN, PARAM_TYPE_UINT32);
    paramIdType[POWER_LINE_FREQ_MODE]
        = std::make_pair(PARAM_ID_POWER_LINE_FREQ_MODE, PARAM_TYPE_UINT32);
    paramIdType[WHITE_BALANCE_TEMPERATURE]
        = std::make_pair(PARAM_ID_WHITE_BALANCE_TEMPERATURE, PARAM_TYPE_UINT32);
    paramIdType[SHARPNESS] = std::make_pair(PARAM_ID_SHARPNESS, PARAM_TYPE_UINT32);
    paramIdType[BACKLIGHT_COMPENSATION]
        = std::make_pair(PARAM_ID_BACKLIGHT_COMPENSATION, PARAM_TYPE_UINT32);
    paramIdType[EXPOSURE_MODE] = std::make_pair(PARAM_ID_EXPOSURE_MODE, PARAM_TYPE_UINT32);
    paramIdType[EXPOSURE_ABSOLUTE] = std::make_pair(PARAM_ID_EXPOSURE_ABSOLUTE, PARAM_TYPE_UINT32);
    paramIdType[IMAGE_SIZE] = std::make_pair(PARAM_ID_IMAGE_SIZE, PARAM_TYPE_UINT32);
    paramIdType[IMAGE_FORMAT] = std::make_pair(PARAM_ID_IMAGE_FORMAT, PARAM_TYPE_UINT32);
    paramIdType[PIXEL_FORMAT] = std::make_pair(PARAM_ID_PIXEL_FORMAT, PARAM_TYPE_UINT32);
    paramIdType[SCENE_MODE] = std::make_pair(PARAM_ID_SCENE_MODE, PARAM_TYPE_UINT32);
    paramIdType[VIDEO_SIZE] = std::make_pair(PARAM_ID_VIDEO_SIZE, PARAM_TYPE_UINT32);
    paramIdType[VIDEO_FRAME_FORMAT]
        = std::make_pair(PARAM_ID_VIDEO_FRAME_FORMAT, PARAM_TYPE_UINT32);
    paramIdType[IMAGE_CAPTURE] = std::make_pair(PARAM_ID_IMAGE_CAPTURE, PARAM_TYPE_UINT32);
    paramIdType[VIDEO_CAPTURE] = std::make_pair(PARAM_ID_VIDEO_CAPTURE, PARAM_TYPE_UINT32);
    paramIdType[VIDEO_SNAPSHOT] = std::make_pair(PARAM_ID_VIDEO_SNAPSHOT, PARAM_TYPE_UINT32);
    paramIdType[IMAGE_VIDEOSHOT] = std::make_pair(PARAM_ID_IMAGE_VIDEOSHOT, PARAM_TYPE_UINT32);
}
