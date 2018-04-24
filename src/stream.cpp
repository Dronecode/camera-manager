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
#include <ostream>
#include <string.h>

#include "stream.h"

std::ostream &operator<<(std::ostream &os, const Stream::FrameSize &fs)
{
    return os << fs.width << "x" << fs.height;
}

std::ostream &operator<<(std::ostream &os, const Stream::PixelFormat &pf)
{
    os << (char)(pf.pixel_format & 0xFF);
    os << (char)(((pf.pixel_format) & 0xFF00) >> 8);
    os << (char)(((pf.pixel_format) & 0xFF0000) >> 16);
    os << (char)(((pf.pixel_format) & 0xFF000000) >> 24);
    os << "(";
    for (unsigned int i = 0; i < pf.frame_sizes.size(); i++) {
        if (i > 0)
            os << ",";
        os << pf.frame_sizes[i];
    }
    os << ")";
    return os;
}

#define fourcc(a, b, c, d) \
    ((uint32_t)(a) << 0) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24)

uint32_t Stream::fourCC(const char *fourcc_str)
{
    assert(fourcc_str);
    assert(strlen(fourcc_str) >= 4);

    return fourcc(fourcc_str[0], fourcc_str[1], fourcc_str[2], fourcc_str[3]);
}

#undef fourcc
