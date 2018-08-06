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
//#include "CameraComponent.h"
#include <functional>
#include <vector>

#include "CameraParameters.h"

/**
 *  The CameraInfo structure is used to hold the camera device information.
 */
struct CameraInfo {
    uint8_t vendorName[32];          /**< Name of the camera device vendor. */
    uint8_t modelName[32];           /**< Name of the camera device model. */
    uint32_t firmware_version;       /**< Firmware version. */
    float focal_length;              /**< Focal length. */
    float sensor_size_h;             /**< Sensor size horizontol. */
    float sensor_size_v;             /**< Sensor size vertical. */
    uint16_t resolution_h;           /**< Resolution horizontol. */
    uint16_t resolution_v;           /**< Resolution vertical. */
    uint8_t lens_id;                 /**< Lens ID. */
    uint32_t flags;                  /**< Flags */
    uint16_t cam_definition_version; /**< Camera Definition file version */
    char cam_definition_uri[140];    /**< Camera Definition file URI address */
};

/**
 *  The CameraData structure is used to hold the camera image data and its
 * meta-data.
 */
struct CameraData {
    uint32_t sec = 0;    /**< system time in sec. */
    uint32_t nsec = 0;   /**< system time in nano sec. */
    uint32_t width = 0;  /**< width of the image. */
    uint32_t height = 0; /**< height of the image. */
    uint32_t stride = 0; /**< stride. */
    //    CameraDevice::PixelFormat pixFmt = 0; /**< pixel format of the image. */
    void *buf = nullptr; /**< buffer address. */
    size_t bufSize = 0;  /**< buffer size. */
};

/**
 *  The CameraDevice class is base for different camera devices.
 */

class CameraDevice {
public:
    CameraDevice() {}
    virtual ~CameraDevice() {}

    /**
     *  Possible return values from class methods.
     */
    enum class Status {
        SUCCESS,
        ERROR_UNKNOWN,
        INVALID_ARGUMENT,
        INVALID_STATE,
        NO_MEMORY,
        PERM_DENIED,
        TIMED_OUT,
        NOT_SUPPORTED
    };

    /**
     *  Possible states of the object.
     */
    enum class State {
        STATE_ERROR,
        STATE_IDLE,
        STATE_INIT,
        STATE_RUN,
    };

    /**
     *  Size of the camera image.
     */
    struct Size {
        uint32_t width;
        uint32_t height;

        Size()
        {
            width = 0;
            height = 0;
        }

        Size(uint32_t w, uint32_t h)
        {
            width = w;
            height = h;
        }
    };

    /**
     *  Get the identifier of the Camera Device.
     *
     *  @return string Camera device Id.
     */
    virtual std::string getDeviceId() const = 0;

    /**
     *  Get camera device information.
     *
     *  @param[out] camInfo Camera Information
     *
     *  @return Status of request.
     */
    virtual Status getInfo(CameraInfo &camInfo) const = 0;

    /**
     *  Check if gstreamer v4l2src element can be used to capture from the Camera Device.
     *
     *  @return bool True if gstreamer v4l2src element supported, otherwise false
     */
    virtual bool isGstV4l2Src() const = 0;

    /**
     *  Initialize camera device.
     *
     *  @param[out] camParam Camera Parameters with default values filled
     *
     *  @return Status of request.
     */
    virtual Status init(CameraParameters &camParam) = 0;

    /**
     *  Uninitialize camera device.
     *
     *  @return Status of request.
     */
    virtual Status uninit() = 0;

    /**
     *  Start camera device.
     *
     *  @return Status of request.
     */
    virtual Status start() { return Status::NOT_SUPPORTED; }

    /**
     *  Start camera device in asynchronous mode.
     *
     *  @param[in] cb Callback function to receive camera data
     *
     *  @return Status of request.
     */
    virtual Status start(const std::function<void(CameraData &)> cb)
    {
        return Status::NOT_SUPPORTED;
    }

    /**
     *  Stop camera device.
     *
     *  @return Status of request.
     */
    virtual Status stop() { return Status::NOT_SUPPORTED; }

    virtual std::vector<uint8_t> read() { return {}; }

    /**
     *  Read camera images from camera device.
     *
     *  @param[out] data CameraData to hold image and meta-data.
     *
     *  @return Status of request.
     */
    virtual Status read(CameraData &data) { return Status::NOT_SUPPORTED; }

    /**
     *  Set parameter of the camera device.
     *
     *  @param[in,out] camParam Camera Parameters.
     *  @param[in] param Name of the parameter.
     *  @param[in] param_value Value of the parameter to be set.
     *  @param[in] value_size Length of the value of the parameter.
     *  @param[in] param_type Type of the parameter.
     *
     *  @return Status of request.
     */
    virtual Status setParam(CameraParameters &camParam, const std::string param,
                            const char *param_value, const size_t value_size, const int param_type)
    {
        return Status::NOT_SUPPORTED;
    }

    /**
     *  Reset parameters of the camera device to default.
     *
     *  @param[in] camParam Camera Parameters.
     *
     *  @return Status of request.
     */
    virtual Status resetParams(CameraParameters &camParam) { return Status::NOT_SUPPORTED; }

    /**
     *  Set camera image resolution.
     *
     *  @param[in] width Width of the output image to be set.
     *  @param[in] height Height of the output image to be set.
     *
     *  @return Status of request.
     */
    virtual Status setSize(const uint32_t width, const uint32_t height)
    {
        return Status::NOT_SUPPORTED;
    }

    /**
     *  Get camera image resolution.
     *
     *  @param[out] width Width of the output image set.
     *  @param[out] height Height of the output image set.
     *
     *  @return Status of request.
     */
    virtual Status getSize(uint32_t &width, uint32_t &height) const
    {
        return Status::NOT_SUPPORTED;
    }

    /**
     *  Get list of supported camera image resolution.
     *
     *  @param[out] sizes Vector of sizes supported.
     *
     *  @return Status of request.
     */
    virtual Status getSupportedSizes(std::vector<Size> &sizes) const
    {
        return Status::NOT_SUPPORTED;
    }

    /**
     *  Set camera image pixel format.
     *
     *  @param[in] format Pixel format of the output image to be set.
     *
     *  @return Status of request.
     */
    virtual Status setPixelFormat(const CameraParameters::PixelFormat format)
    {
        return Status::NOT_SUPPORTED;
    }

    /**
     *  Get camera image pixel format.
     *
     *  @param[out] format Pixel format of the output image set.
     *
     *  @return Status of request.
     */
    virtual Status getPixelFormat(CameraParameters::PixelFormat &format) const
    {
        return Status::NOT_SUPPORTED;
    }

    /**
     *  Get supported camera image pixel format.
     *
     *  @param[out] formats Vector of PixelFormat supported by the camera device.
     *
     *  @return Status of request.
     */
    virtual Status
    getSupportedPixelFormats(std::vector<CameraParameters::PixelFormat> &formats) const
    {
        return Status::NOT_SUPPORTED;
    }

    /**
     *  Set mode of the camera device.
     *
     *  @param[in] mode Camera device mode to be set.
     *
     *  @return Status of request.
     */
    virtual Status setMode(const CameraParameters::Mode mode) { return Status::NOT_SUPPORTED; }

    /**
     *  Get mode of the camera device.
     *
     *  @param[out] mode Camera device mode that is set.
     *
     *  @return Status of request.
     */
    virtual Status getMode(CameraParameters::Mode &mode) const { return Status::NOT_SUPPORTED; }

    /**
     *  Get supported modes of the camera device.
     *
     *  @param[out] modes Camera device mode that are supported.
     *
     *  @return Status of request.
     */
    virtual Status getSupportedModes(std::vector<CameraParameters::Mode> &modes) const
    {
        return Status::NOT_SUPPORTED;
    }

    /**
     *  Set frame rate of the camera device.
     *
     *  @param[in] fps Frame rate per second of the camera device to be set.
     *
     *  @return Status of request.
     */
    virtual Status setFrameRate(const uint32_t fps) { return Status::NOT_SUPPORTED; }

    /**
     *  Get frame rate of the camera device.
     *
     *  @param[out] fps Frame rate per second of the camera device.
     *
     *  @return Status of request.
     */
    virtual Status getFrameRate(uint32_t &fps) const { return Status::NOT_SUPPORTED; }

    /**
     *  Get range of frame rate supported by the camera device.
     *
     *  @param[out] minFps Minimum frame rate per second that can be set.
     *  @param[out] maxFps Maximum frame rate per second that can be set.
     *
     *  @return Status of request.
     */
    virtual Status getSupportedFrameRates(uint32_t &minFps, uint32_t &maxFps) const
    {
        return Status::NOT_SUPPORTED;
    }

    /**
     *  Set URI of the camera definition file for the camera device.
     *
     *  @param[in] uri String with absolute address of the camera definition file.
     *
     *  @return Status of request.
     */
    virtual Status setCameraDefinitionUri(const std::string uri) { return Status::NOT_SUPPORTED; }

    /**
     *  Get URI of the camera definition file for the camera device.
     *  Either the camera device has some predefined/generated URI address OR received by the set
     * function
     *
     *  @return string URI address of the camera definition file.
     */
    virtual std::string getCameraDefinitionUri() const { return {}; }

    /**
     *  Get text that needs to be overlayed on camera image frames.
     *  This may be helpful where some text information can be overlayed on camera images read from
     *  the device and displayed to the user. For example, on changing the brighntess parameter to
     *  100, a text like "Brightness = 100" can be displayed on the video that is getting streamed.
     *
     *  @return string Overlay text.
     */
    virtual std::string getOverlayText() const { return {}; };
};
