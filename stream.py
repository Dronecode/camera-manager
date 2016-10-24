#!/usr/bin/env python

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

import dbus
import dbus.service
import dbus.mainloop.glib
import fcntl
import glob
import ipaddress
import v4l2_defines

from gi import require_version
require_version("Gst", "1.0")
require_version("GstRtspServer", "1.0")
from gi.repository import Gst, GstRtspServer, GObject, GLib

STREAM_MANAGER = None

CAMERA_IFACE = "org.ardupilot.camera"
STREAM_IFACE = "org.ardupilot.camera.stream"
MANAGER_IFACE = "org.ardupilot.StreamManager"

class Stream(dbus.service.Object):
    _cameraPath = ''
    _objectPath = ''

    _width = 0
    _height = 0
    _streamFormat = ''
    _mountPath = ''
    _pipelineDescription = ''

    _capabilitiesDict = {
        v4l2_defines.V4L2_CAP_VIDEO_CAPTURE: "V4L2_CAP_VIDEO_CAPTURE",
        v4l2_defines.V4L2_CAP_VIDEO_OUTPUT: "V4L2_CAP_VIDEO_OUTPUT",
        v4l2_defines.V4L2_CAP_VIDEO_OVERLAY: "V4L2_CAP_VIDEO_OVERLAY",
        v4l2_defines.V4L2_CAP_VBI_CAPTURE: "V4L2_CAP_VBI_CAPTURE",
        v4l2_defines.V4L2_CAP_VBI_OUTPUT: "V4L2_CAP_VBI_OUTPUT",
        v4l2_defines.V4L2_CAP_SLICED_VBI_CAPTURE: "V4L2_CAP_SLICED_VBI_CAPTURE",
        v4l2_defines.V4L2_CAP_SLICED_VBI_OUTPUT: "V4L2_CAP_SLICED_VBI_OUTPUT",
        v4l2_defines.V4L2_CAP_RDS_CAPTURE: "V4L2_CAP_RDS_CAPTURE",
        v4l2_defines.V4L2_CAP_VIDEO_OUTPUT_OVERLAY: "V4L2_CAP_VIDEO_OUTPUT_OVERLAY",
        v4l2_defines.V4L2_CAP_HW_FREQ_SEEK: "V4L2_CAP_HW_FREQ_SEEK",
        v4l2_defines.V4L2_CAP_RDS_OUTPUT: "V4L2_CAP_RDS_OUTPUT",
        v4l2_defines.V4L2_CAP_TUNER: "V4L2_CAP_TUNER",
        v4l2_defines.V4L2_CAP_AUDIO: "V4L2_CAP_AUDIO",
        v4l2_defines.V4L2_CAP_RADIO: "V4L2_CAP_RADIO",
        v4l2_defines.V4L2_CAP_MODULATOR: "V4L2_CAP_MODULATOR",
        v4l2_defines.V4L2_CAP_READWRITE: "V4L2_CAP_READWRITE",
        v4l2_defines.V4L2_CAP_ASYNCIO: "V4L2_CAP_ASYNCIO",
        v4l2_defines.V4L2_CAP_STREAMING: "V4L2_CAP_STREAMING",
    }

    _formatsPipelineElements = {
        # RGB formats
        "RGB1": {}, # caps: video/x-raw, format=BGRx
        "R444": {}, # caps: video/x-raw, format=BGRx
        "RGB0": {},
        "RGBP": {},
        "RGBQ": {},
        "RGBR": {},
        "BGR3": {}, # caps: video/x-raw, format=BGRx
        "RGB3": {}, # caps: video/x-raw, format=BGRx
        "BGR4": {}, # caps: video/x-raw, format=BGRx
        "RGB4": {}, # caps: video/x-raw, format=BGRx
        # Grey formats
        "GREY": { "caps": "video/x-raw, format=GREY8", "converter": "videoconvert", "encoder": "jpegenc", "muxer": "rtpjpegpay" },
        "Y10":  {},
        "Y16":  { "caps": "video/x-raw, format=GREY16_LE", "converter": "videoconvert", "encoder": "jpegenc", "muxer": "rtpjpegpay" },
        # Palette formats
        "PAL8": {},
        # Luminance+Chrominance formats
        "YVU9": { "caps": "video/x-raw, format=YVU9", "converter": "videoconvert", "encoder": "jpegenc", "muxer": "rtpjpegpay" },
        "YV12": { "caps": "video/x-raw, format=YV12", "converter": "videoconvert", "encoder": "jpegenc", "muxer": "rtpjpegpay" },
        "YUYV": { "caps": "video/x-raw, format=YUY2", "converter": "videoconvert", "encoder": "jpegenc", "muxer": "rtpjpegpay" },
        "YYUV": {},
        "YVYU": { "caps": "video/x-raw, format=YVYU", "converter": "videoconvert", "encoder": "jpegenc", "muxer": "rtpjpegpay" },
        "UYVY": { "caps": "video/x-raw, format=UYVY", "converter": "videoconvert", "encoder": "jpegenc", "muxer": "rtpjpegpay" },
        "VYUY": {},
        "422P": { "caps": "video/x-raw, format=Y42B", "converter": "videoconvert", "encoder": "jpegenc", "muxer": "rtpjpegpay" },
        "411P": { "caps": "video/x-raw, format=Y41B", "converter": "videoconvert", "encoder": "jpegenc", "muxer": "rtpjpegpay" },
        "Y41P": { "caps": "video/x-raw, format=Y41B", "converter": "videoconvert", "encoder": "jpegenc", "muxer": "rtpjpegpay" },
        "Y444": {},
        "YUVO": {},
        "YUVP": {},
        "YUV4": {},
        "YUV9": {},
        "YU12": {},
        "HI24": {},
        "HM12": {},
        # Two planes -- one Y, one Cr + Cb interleaved
        "NV12": { "caps": "video/x-raw, format=NV12", "encoder": "x264enc", "muxer": "rtph264pay" },
        "NV21": { "caps": "video/x-raw, format=NV21", "encoder": "x264enc", "muxer": "rtph264pay" },
        "NV16": { "caps": "video/x-raw, format=NV16", "encoder": "x264enc", "muxer": "rtph264pay" },
        "NV61": { "caps": "video/x-raw, format=NV61", "encoder": "x264enc", "muxer": "rtph264pay" },
        # Bayer formats
        "BA81": {},
        "GBRG": {}, # caps: video/x-bayer, format=gbrg
        "GRBG": {}, # caps: video/x-bayer, format=grbg
        "RGGB": {}, # caps: video/x-bayer, format=rggb
        "BG10": {},
        "GB10": {},
        "BA10": {},
        "RG10": {},
        "BD10": {},
        "BYR2": {},
        # Compressed formats
        "MJPG": { "caps": "image/jpeg", "muxer": "rtpjpegpay" },
        "JPEG": { "caps": "image/jpeg", "muxer": "rtpjpegpay" },
        "dvsd": { "caps": "video/x-dv", "muxer": "rtpdvpay" },
        "MPEG": { "caps": "video/mpeg", "muxer": "rtpmp4vpay" },
        # Vendor-specific formats
        "CPIA": {},
        "WNVA": {},
        "S910": {},
        "S920": {},
        "PWC1": {}, # caps: video/x-pwc1
        "PWC2": {}, # caps: video/x-pwc2
        "E625": {},
        "S501": {},
        "S505": {},
        "S508": {},
        "S561": {},
        "P207": {},
        "M310": {},
        "SONX": {}, # caps: video/x-sonix
        "905C": {},
        "PJPG": {},
        "0511": {},
        "0518": {},
        "S680": {},
    }

    def __init__(self, conn, cameraPath, streamFormat, mountPath, width=0, height=0):
        self._cameraPath = cameraPath
        self._objectPath = '/org/ardupilot/stream' + mountPath
        self._streamFormat = streamFormat
        self._mountPath = mountPath
        self._width = width
        self._height = height

        self._pipelineDescription = self.BuildPipelineDescription(streamFormat)
        if not self._pipelineDescription:
            raise ValueError

        dbus.service.Object.__init__(self, conn, self._objectPath)

    def BuildPipelineDescription(self, streamFormat):
        if not "V4L2_CAP_STREAMING" in self.GetCapabilities().get('capabilities'):
            print("Error: Streaming is not supported in '" + self._cameraPath + "'")
            return None

        if not streamFormat in self.GetFormats():
            print("Error: Camera format '" + streamFormat + "' not available for '" + self._cameraPath + "'")
            return None

        elem = Stream._formatsPipelineElements.get(streamFormat)
        if not elem:
            print("Error: Camera format '" + streamFormat + "' without support")
            return None

        desc = "v4l2src device=" + self._cameraPath

        if elem.get("caps"):
            desc += " ! " + elem.get("caps")

        # Check if frame size was set directly, if set we use these values,
        # otherwise we just let the camera use its default frame size
        if self._width and self._height:
            desc += ", width=" + str(self._width) + ", height=" + str(self._height)

        if elem.get("converter"):
            desc += " ! " + elem.get("converter")

        if elem.get("encoder"):
            desc += " ! " + elem.get("encoder")

        if elem.get("muxer"):
            desc += " ! " + elem.get("muxer") + " name=pay0"

        return desc

    def GetPath(self):
        return self._objectPath

    def GetMountPath(self):
        return self._mountPath

    def GetPipelineDescription(self):
        return self._pipelineDescription

    def PixelFormatToFourcc(i):
        return chr(i & 0xFF) + chr((i & 0xFF00) >> 8) + chr((i & 0xFF0000) >> 16) + chr((i & 0xFF000000) >> 24)

    def GetFormatsImpl(path):
        try:
            v = open(path, "r")
        except Exception as e:
            print("Error: Couldn't get camera '" + path + "' capabilities: " + str(e))
            return {}

        c = v4l2_defines.v4l2_fmtdesc()
        c.type = v4l2_defines.V4L2_BUF_TYPE_VIDEO_CAPTURE

        formats = []
        try:
            while fcntl.ioctl(v, v4l2_defines.VIDIOC_ENUM_FMT, c) == 0:
                # Some cameras have custom formats, by now we won't deal with them.
                if c.pixelformat:
                    formats.append(Stream.PixelFormatToFourcc(c.pixelformat))
                c.index = c.index + 1
        except IOError: # After the last index, it will give us an IOError
            pass

        return formats

    def GetCapabilitiesNames(capabilities):
        capabilitiesNames = []

        for key, value in Stream._capabilitiesDict.items():
            if capabilities & key:
                capabilitiesNames.append(value)

        return capabilitiesNames

    @dbus.service.method(CAMERA_IFACE, out_signature="a{sv}")
    def GetCapabilities(self):
        try:
            v = open(self._cameraPath, "r")
            c = v4l2_defines.v4l2_capability()
            fcntl.ioctl(v, v4l2_defines.VIDIOC_QUERYCAP, c)
        except Exception as e:
            print("Error: Couldn't get camera '" + self._cameraPath + "' capabilities: " + str(e))
            return {}

        return { "driver": c.driver.decode("utf-8"),
                 "card": c.card.decode("utf-8"),
                 "bus_info": c.bus_info.decode("utf-8"),
                 "version": str((c.version >> 16) & 0xFF) + "." +
                            str((c.version >> 8) & 0xFF) + "." +
                            str(c.version & 0xFF),
                 "capabilities_union": str(c.capabilities),
                 "capabilities": Stream.GetCapabilitiesNames(c.capabilities) }

    @dbus.service.method(CAMERA_IFACE, out_signature="as")
    def GetFormats(self):
        return Stream.GetFormatsImpl(self._cameraPath)

    @dbus.service.method(CAMERA_IFACE, out_signature="s")
    def GetPath(self):
        return self._cameraPath

    @dbus.service.method(STREAM_IFACE, out_signature="s")
    def GetURI(self):
        return "rtsp://" + STREAM_MANAGER.GetIP()+ ":" + STREAM_MANAGER.GetPort() + self._mountPath

    @dbus.service.method(STREAM_IFACE)
    def Remove(self):
        STREAM_MANAGER.RemoveStream(self._mountPath)

    @dbus.service.method(STREAM_IFACE, out_signature="s")
    def GetFormat(self):
        return self._streamFormat

    @dbus.service.method(STREAM_IFACE, out_signature="ai")
    def GetFrameSize(self):
        try:
            v = open(self._cameraPath, "r")
        except Exception as e:
            print("Error: Couldn't get camera '" + self._cameraPath + "' frame size: " + str(e))
            return []

        fmt = v4l2_defines.v4l2_format()
        fmt.type = v4l2_defines.V4L2_BUF_TYPE_VIDEO_CAPTURE

        try:
            if fcntl.ioctl(v, v4l2_defines.VIDIOC_G_FMT, fmt) == 0:
                return [fmt.fmt.pix.width, fmt.fmt.pix.height]
        except Exception as e:
            print("Error: Couldn't get camera '" + self._cameraPath + "' frame size: " + str(e))

        return []

    def SetStream(self, streamFormat, mountPath, width, height):
        if not self.BuildPipelineDescription(streamFormat):
            return False

        self.Remove()

        if mountPath[0] != '/':
            mountPath = '/' + mountPath

        STREAM_MANAGER.AddStreamImpl(self._cameraPath, streamFormat, mountPath, width, height)

        return True

    @dbus.service.method(STREAM_IFACE, in_signature="s", out_signature="b")
    def SetFormat(self, streamFormat):
        return self.SetStream(streamFormat, self._mountPath, self._width, self._height)

    @dbus.service.method(STREAM_IFACE, in_signature="s", out_signature="b")
    def SetMountPath(self, mountPath):
        return self.SetStream(self._streamFormat, mountPath, self._width, self._height)

    @dbus.service.method(STREAM_IFACE, in_signature="ii", out_signature="b")
    def SetFrameSize(self, width, height):
        return self.SetStream(self._streamFormat, self._mountPath, width, height)

class StreamManager(dbus.service.Object):
    _bus = None
    _streams = []

    _rtspSourceID = 0
    _rtspServer = None
    _rtspMountPoints = None

    def __init__(self, conn):
        self._bus = conn

        self._rtspServer = GstRtspServer.RTSPServer.new()
        self._rtspServer.set_address("0.0.0.0")
        self._rtspServer.set_service("8554")

        self._rtspMountPoints = self._rtspServer.get_mount_points()

        dbus.service.Object.__init__(self, conn, "/org/ardupilot/StreamManager")

    def StartStreams(self):
        for cameraPath in glob.glob("/dev/video*"):
            for cameraFormat in Stream.GetFormatsImpl(cameraPath):
                formatElements = Stream._formatsPipelineElements.get(cameraFormat)
                if not formatElements:
                    continue # format not supported

                streamFormat = cameraFormat
                if not formatElements.get("converter"):
                    break # formats without converter are preferred

            if streamFormat:
                self.AddStream(cameraPath, streamFormat, "/stream" + cameraPath[10:])

    @dbus.service.method(MANAGER_IFACE, out_signature="as")
    def GetCameras(self):
        return glob.glob("/dev/video*")

    @dbus.service.method(MANAGER_IFACE, out_signature="as")
    def GetStreams(self):
        return [s.GetMountPath() for s in self._streams]

    def AddStreamImpl(self, cameraPath, streamFormat, mountPath, width=0, height=0):
        if not mountPath:
            print("Error: Mount path should not be empty")
            return False

        if mountPath[0] != '/':
            mountPath = '/' + mountPath

        if not cameraPath in self.GetCameras():
            print("Error: Invalid camera path '" + cameraPath)
            return False

        try:
            stream = Stream(self._bus, cameraPath, streamFormat, mountPath, width, height)
        except ValueError:
            return False

        factory = GstRtspServer.RTSPMediaFactory.new()
        factory.set_launch(stream.GetPipelineDescription())

        self._rtspMountPoints.add_factory(stream.GetMountPath(), factory)

        if self._rtspSourceID > 0:
            GLib.Source.remove(self._rtspSourceID)

        self._rtspSourceID = self._rtspServer.attach(None)

        self._streams.append(stream)

        return True

    @dbus.service.method(MANAGER_IFACE, in_signature="sss", out_signature="b")
    def AddStream(self, cameraPath, streamFormat, mountPath):
        return self.AddStreamImpl(cameraPath, streamFormat, mountPath)

    @dbus.service.method(MANAGER_IFACE, in_signature="s", out_signature="b")
    def RemoveStream(self, mountPath):
        if mountPath[0] != '/':
            mountPath = '/' + mountPath

        stream = None
        for s in self._streams:
            if s.GetMountPath() == mountPath:
                stream = s

        if not stream:
            return False

        self._streams.remove(stream)
        stream.remove_from_connection()

        # The stream will keep alive until there's no one connected.
        self._rtspMountPoints.remove_factory(mountPath)

        return True

    @dbus.service.method(MANAGER_IFACE, in_signature="s", out_signature="b")
    def SetIP(self, ip):
        try:
            ipaddress.ip_address(ip)
        except ValueError as e:
            print("Error: Incorrect IP format " + str(e))
            return False

        self._rtspServer.set_address(ip)

        if self._rtspSourceID > 0:
           GLib.Source.remove(self._rtspSourceID)

        self._rtspSourceID = self._rtspServer.attach(None)

        return True

    @dbus.service.method(MANAGER_IFACE, in_signature="i", out_signature="b")
    def SetPort(self, port):
        if port < 0 or port > 65535:
            print("Error: Port number out of range 0...65535")
            return False

        self._rtspServer.set_service(str(port))

        if self._rtspSourceID > 0:
           GLib.Source.remove(self._rtspSourceID)

        self._rtspSourceID = self._rtspServer.attach(None)

        return True

    @dbus.service.method(MANAGER_IFACE, out_signature="s")
    def GetIP(self):
        return self._rtspServer.get_address()

    @dbus.service.method(MANAGER_IFACE, out_signature="s")
    def GetPort(self):
        return self._rtspServer.get_service()

if __name__ == "__main__":
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    systemBus = dbus.SystemBus()

    Gst.init(None)

    name = dbus.service.BusName(STREAM_IFACE, systemBus)

    STREAM_MANAGER = StreamManager(systemBus)

    STREAM_MANAGER.StartStreams()

    GObject.MainLoop().run()
