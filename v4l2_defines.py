# Copyright (C) 2016 Intel Corporation. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import ctypes

V4L2_CAP_VIDEO_CAPTURE        = 0x00000001
V4L2_CAP_VIDEO_OUTPUT         = 0x00000002
V4L2_CAP_VIDEO_OVERLAY        = 0x00000004
V4L2_CAP_VBI_CAPTURE          = 0x00000010
V4L2_CAP_VBI_OUTPUT           = 0x00000020
V4L2_CAP_SLICED_VBI_CAPTURE   = 0x00000040
V4L2_CAP_SLICED_VBI_OUTPUT    = 0x00000080
V4L2_CAP_RDS_CAPTURE          = 0x00000100
V4L2_CAP_VIDEO_OUTPUT_OVERLAY = 0x00000200
V4L2_CAP_HW_FREQ_SEEK         = 0x00000400
V4L2_CAP_RDS_OUTPUT           = 0x00000800
V4L2_CAP_TUNER                = 0x00010000
V4L2_CAP_AUDIO                = 0x00020000
V4L2_CAP_RADIO                = 0x00040000
V4L2_CAP_MODULATOR            = 0x00080000
V4L2_CAP_READWRITE            = 0x01000000
V4L2_CAP_ASYNCIO              = 0x02000000
V4L2_CAP_STREAMING            = 0x04000000

VIDIOC_QUERYCAP = -2140645888
VIDIOC_ENUM_FMT = -1069525502
VIDIOC_G_FMT    = -1060088316

v4l2_buf_type = ctypes.c_uint
(
    V4L2_BUF_TYPE_VIDEO_CAPTURE,
    V4L2_BUF_TYPE_VIDEO_OUTPUT,
    V4L2_BUF_TYPE_VIDEO_OVERLAY,
    V4L2_BUF_TYPE_VBI_CAPTURE,
    V4L2_BUF_TYPE_VBI_OUTPUT,
    V4L2_BUF_TYPE_SLICED_VBI_CAPTURE,
    V4L2_BUF_TYPE_SLICED_VBI_OUTPUT,
    V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY,
    V4L2_BUF_TYPE_PRIVATE,
) = list(range(1, 9)) + [0x80]

v4l2_field = ctypes.c_uint
(
    V4L2_FIELD_ANY,
    V4L2_FIELD_NONE,
    V4L2_FIELD_TOP,
    V4L2_FIELD_BOTTOM,
    V4L2_FIELD_INTERLACED,
    V4L2_FIELD_SEQ_TB,
    V4L2_FIELD_SEQ_BT,
    V4L2_FIELD_ALTERNATE,
    V4L2_FIELD_INTERLACED_TB,
    V4L2_FIELD_INTERLACED_BT,
) = range(10)

v4l2_colorspace = ctypes.c_uint
(
    V4L2_COLORSPACE_SMPTE170M,
    V4L2_COLORSPACE_SMPTE240M,
    V4L2_COLORSPACE_REC709,
    V4L2_COLORSPACE_BT878,
    V4L2_COLORSPACE_470_SYSTEM_M,
    V4L2_COLORSPACE_470_SYSTEM_BG,
    V4L2_COLORSPACE_JPEG,
    V4L2_COLORSPACE_SRGB,
) = range(1, 9)

class v4l2_fmtdesc(ctypes.Structure):
    _fields_ = [
        ('index', ctypes.c_uint32),
        ('type', ctypes.c_int),
        ('flags', ctypes.c_uint32),
        ('description', ctypes.c_char * 32),
        ('pixelformat', ctypes.c_uint32),
        ('reserved', ctypes.c_uint32 * 4),
    ]

class v4l2_capability(ctypes.Structure):
    _fields_ = [
        ('driver', ctypes.c_char * 16),
        ('card', ctypes.c_char * 32),
        ('bus_info', ctypes.c_char * 32),
        ('version', ctypes.c_uint32),
        ('capabilities', ctypes.c_uint32),
        ('reserved', ctypes.c_uint32 * 4),
    ]

class v4l2_pix_format(ctypes.Structure):
    _fields_ = [
        ('width', ctypes.c_uint32),
        ('height', ctypes.c_uint32),
        ('pixelformat', ctypes.c_uint32),
        ('field', v4l2_field),
        ('bytesperline', ctypes.c_uint32),
        ('sizeimage', ctypes.c_uint32),
        ('colorspace', v4l2_colorspace),
        ('priv', ctypes.c_uint32),
    ]

class v4l2_rect(ctypes.Structure):
    _fields_ = [
        ('left', ctypes.c_int32),
        ('top', ctypes.c_int32),
        ('width', ctypes.c_int32),
        ('height', ctypes.c_int32),
    ]

class v4l2_clip(ctypes.Structure):
    pass
v4l2_clip._fields_ = [
    ('c', v4l2_rect),
    ('next', ctypes.POINTER(v4l2_clip)),
]

class v4l2_window(ctypes.Structure):
    _fields_ = [
        ('w', v4l2_rect),
        ('field', v4l2_field),
        ('chromakey', ctypes.c_uint32),
        ('clips', ctypes.POINTER(v4l2_clip)),
        ('clipcount', ctypes.c_uint32),
        ('bitmap', ctypes.c_void_p),
        ('global_alpha', ctypes.c_uint8),
    ]

class v4l2_vbi_format(ctypes.Structure):
    _fields_ = [
        ('sampling_rate', ctypes.c_uint32),
        ('offset', ctypes.c_uint32),
        ('samples_per_line', ctypes.c_uint32),
        ('sample_format', ctypes.c_uint32),
        ('start', ctypes.c_int32 * 2),
        ('count', ctypes.c_uint32 * 2),
        ('flags', ctypes.c_uint32),
        ('reserved', ctypes.c_uint32 * 2),
    ]

class v4l2_sliced_vbi_format(ctypes.Structure):
    _fields_ = [
        ('service_set', ctypes.c_uint16),
        ('service_lines', ctypes.c_uint16 * 2 * 24),
        ('io_size', ctypes.c_uint32),
        ('reserved', ctypes.c_uint32 * 2),
    ]

class v4l2_format(ctypes.Structure):
    class _u(ctypes.Union):
        _fields_ = [
            ('pix', v4l2_pix_format),
            ('win', v4l2_window),
            ('vbi', v4l2_vbi_format),
            ('sliced', v4l2_sliced_vbi_format),
            ('raw_data', ctypes.c_char * 200),
        ]

    _fields_ = [
        ('type', v4l2_buf_type),
        ('fmt', _u),
    ]
