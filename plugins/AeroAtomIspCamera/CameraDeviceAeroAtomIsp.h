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
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "CameraDevice.h"
#include "CameraParameters.h"

class CameraDeviceAeroAtomIsp final : public CameraDevice {
public:
    CameraDeviceAeroAtomIsp(std::string device);
    ~CameraDeviceAeroAtomIsp();
    std::string getDeviceId() const;
    Status getInfo(CameraInfo &camInfo) const;
    bool isGstV4l2Src() const;
    Status init(CameraParameters &camParam);
    Status uninit();
    Status start();
    Status stop();
    Status read(CameraData &data);
    Status setParam(CameraParameters &camParam, const std::string param, const char *param_value,
                    const size_t value_size, const int param_type);
    Status resetParams(CameraParameters &camParam);
    Status setSize(const uint32_t width, const uint32_t height);
    Status getSize(uint32_t &width, uint32_t &height) const;
    Status getSupportedSizes(std::vector<Size> &sizes) const;
    Status setPixelFormat(const CameraParameters::PixelFormat format);
    Status getPixelFormat(CameraParameters::PixelFormat &format) const;
    Status getSupportedPixelFormats(std::vector<CameraParameters::PixelFormat> &formats) const;
    Status setMode(const CameraParameters::Mode mode);
    Status getMode(CameraParameters::Mode &mode) const;
    Status getSupportedModes(std::vector<CameraParameters::Mode> &modes) const;
    Status setFrameRate(const uint32_t fps);
    Status getFrameRate(uint32_t &fps) const;
    Status getSupportedFrameRates(uint32_t &minFps, uint32_t &maxFps) const;
    Status setCameraDefinitionUri(const std::string uri);
    std::string getCameraDefinitionUri() const;

private:
    CameraDevice::Status init();
    Status setState(const CameraDevice::State state);
    CameraDevice::State getState() const;
    int allocFrameBuffer(int bufCnt, size_t bufSize);
    int freeFrameBuffer();
    int pollCamera(int fd);
    std::string mDeviceId;
    std::atomic<CameraDevice::State> mState;
    uint32_t mWidth;
    uint32_t mHeight;
    CameraParameters::PixelFormat mPixelFormat;
    CameraParameters::Mode mMode;
    uint32_t mFrmRate;
    std::string mCamDefUri;
    std::mutex mLock;
    int mFd;
    void **mFrameBuffer;
    size_t mFrameBufferSize;
    uint32_t mFrameBufferCnt;
};
