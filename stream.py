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

import fcntl
import glob
import ipaddress
import os
import v4l2_defines
import socket

os.environ['MAVLINK20'] = '1' #Force using mavlink 2.0
from pymavlink import mavutil

from argparse import ArgumentParser, ArgumentTypeError
from serial import serialutil
from threading import Thread
from time import sleep

from gi import require_version
require_version("Gst", "1.0")
require_version("GstRtspServer", "1.0")
from gi.repository import Gst, GstRtspServer, GObject, GLib

class Stream():
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

    def __init__(self, cameraPath, streamFormat, mountPath, width=0, height=0):
        self._cameraPath = cameraPath
        self._streamFormat = streamFormat
        self._mountPath = mountPath
        self._width = width
        self._height = height

        self._pipelineDescription = self.BuildPipelineDescription(streamFormat)
        if not self._pipelineDescription:
            raise ValueError

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

    def GetID(self):
        return int(self._cameraPath[10:])

    def GetName(self):
        return self._cameraPath[5:]

    def GetMountPath(self):
        return self._mountPath

    def GetPipelineDescription(self):
        return self._pipelineDescription

    @staticmethod
    def PixelFormatToFourcc(i):
        return chr(i & 0xFF) + chr((i & 0xFF00) >> 8) + chr((i & 0xFF0000) >> 16) + chr((i & 0xFF000000) >> 24)

    @staticmethod
    def FourccToPixelFormat(f):
        a, b, c, d = bytearray(f.encode())
        return (a << 0) | (b << 8) | (c << 16) | (d << 24)

    @staticmethod
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

    @staticmethod
    def GetCapabilitiesNames(capabilities):
        capabilitiesNames = []

        for key, value in Stream._capabilitiesDict.items():
            if capabilities & key:
                capabilitiesNames.append(value)

        return capabilitiesNames

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

    def GetFormats(self):
        return Stream.GetFormatsImpl(self._cameraPath)

    def GetPath(self):
        return self._cameraPath

    def GetURI(self, address=None):
        if not address:
            address = stream_manager.GetIP()
        return "rtsp://" + address + ":" + stream_manager.GetPort() + self._mountPath

    def Remove(self):
        stream_manager.RemoveStream(self._mountPath)

    def GetFormat(self):
        return self._streamFormat

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
        pipeline = self.BuildPipelineDescription(streamFormat)
        if not pipeline:
            return False

        factory, match = stream_manager._rtspServer.get_mount_points().match(mountPath)
        if not factory:
            return False
        factory.set_launch(pipeline)

        self._streamFormat = streamFormat
        self._mountPath = mountPath
        self._width = width
        self._height = height

        return True

    def SetFormat(self, streamFormat):
        return self.SetStream(streamFormat, self._mountPath, self._width, self._height)

    def SetMountPath(self, mountPath):
        return self.SetStream(self._streamFormat, mountPath, self._width, self._height)

    def SetFrameSize(self, width, height):
        return self.SetStream(self._streamFormat, self._mountPath, width, height)

class StreamManager():
    _streams = []

    _rtspSourceID = 0
    _rtspServer = None
    _rtspMountPoints = None

    def __init__(self):
        self._rtspServer = GstRtspServer.RTSPServer.new()
        self._rtspServer.set_address("0.0.0.0")
        self._rtspServer.set_service("8554")

        self._rtspMountPoints = self._rtspServer.get_mount_points()

    def StartStreams(self):
        for cameraPath in sorted(glob.glob("/dev/video*")):
            streamFormat = None
            for cameraFormat in Stream.GetFormatsImpl(cameraPath):
                formatElements = Stream._formatsPipelineElements.get(cameraFormat)
                if not formatElements:
                    continue # format not supported

                if not formatElements.get("converter"):
                    streamFormat = cameraFormat
                    break # formats without converter are preferred
                if not streamFormat:
                    streamFormat = cameraFormat


            if streamFormat:
                self.AddStream(cameraPath, streamFormat, "/stream" + cameraPath[10:])

    def GetStreamObjects(self):
        return self._streams

    def GetCameras(self):
        return glob.glob("/dev/video*")

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
            stream = Stream(cameraPath, streamFormat, mountPath, width, height)
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

    def AddStream(self, cameraPath, streamFormat, mountPath):
        return self.AddStreamImpl(cameraPath, streamFormat, mountPath)

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

        # The stream will keep alive until there's no one connected.
        self._rtspMountPoints.remove_factory(mountPath)

        return True

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

    def SetPort(self, port):
        if port < 0 or port > 65535:
            print("Error: Port number out of range 0...65535")
            return False

        self._rtspServer.set_service(str(port))

        if self._rtspSourceID > 0:
           GLib.Source.remove(self._rtspSourceID)

        self._rtspSourceID = self._rtspServer.attach(None)

        return True

    def GetIP(self):
        return self._rtspServer.get_address()

    def GetPort(self):
        return self._rtspServer.get_service()

class MavlinkManager():
    def __init__(self, stream_manager):
        self.stream_manager = stream_manager

    def setup_mavlink_connection(self, args):
        device_parts = args.device.split(':')
        if device_parts[0] == "udp":
            mavlink_connection = mavutil.mavlink_connection(args.target)

            mavlink_connection.port.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            mavlink_connection.port.bind((device_parts[1], 0))
            self.address = device_parts[1]
        else:
            mavlink_connection = mavutil.mavlink_connection(args.device, baud=args.baud)

        mavlink_connection.mav.srcSystem = int(args.system_id)
        mavlink_connection.mav.srcComponent = mavutil.mavlink.MAV_COMP_ID_VIDEO_STREAM

        return mavlink_connection

    def Start(self):
        try:
            self.mavlink_connection = self.setup_mavlink_connection(args)
        except serialutil.SerialException:
            print('Could not connect to device "' + args.device + '"')
            os._exit(1)

        hbthread = Thread(target=self.hbloop)
        hbthread.daemon = True
        hbthread.start()

        mavthread = Thread(target=self.mavloop)
        mavthread.daemon = True
        mavthread.start()

    def handle_video_stream_get(self, msg):
        if msg.command == mavutil.mavlink.VIDEO_STREAM_GET_CMD_STREAMS:
            for s in stream_manager.GetStreamObjects():
                if msg.id != 0 and s.GetID() != msg.id:
                    continue

                self.mavlink_connection.mav.video_stream_uri_send(s.GetID(), s.GetName().encode(),
                    s.GetURI(address = self.address).encode())

        elif msg.command == mavutil.mavlink.VIDEO_STREAM_GET_CMD_STREAM_SETTINGS:
            for s in stream_manager.GetStreamObjects():
                if msg.id != 0 and s.GetID() != msg.id:
                    continue

                capabilities = s.GetCapabilities()
                if not capabilities:
                    break
                capabilities = int(capabilities['capabilities_union'])

                format = s.GetFormat()
                if not format:
                    break
                format = Stream.FourccToPixelFormat(format)

                available_formats = s.GetFormats()
                if not available_formats:
                    break

                formats_array = [0] * 20
                for i, item in enumerate(available_formats):
                    if i >= 20:
                        break
                    formats_array[i] = Stream.FourccToPixelFormat(item)

                frame_size = s.GetFrameSize()
                if not frame_size:
                    break

                self.mavlink_connection.mav.video_stream_settings_send(s.GetID(),
                    s.GetName().encode(), capabilities,
                    format, formats_array, frame_size[0], frame_size[1], s.GetURI(address = self.address).encode())

                break

    def handle_set_video_stream_settings(self, msg):
        for s in stream_manager.GetStreamObjects():
            if s.GetID() != msg.id:
                continue

            msg_format = s.GetFormat()
            msg_height = s.GetFrameSize()[1]
            msg_width = s.GetFrameSize()[0]
            msg_mount_path = s.GetMountPath()
            if msg.format:
                msg_format = Stream.PixelFormatToFourcc(msg.format)

            if msg.video_resolution_h and msg.video_resolution_v:
                msg_height = msg.video_resolution_h
                msg_width = msg.video_resolution_v

            mount_path = str(msg.mount_path.decode("utf-8"))
            if mount_path:
                mount_path = msg_mount_path

            s.SetStream(msg_format, msg_mount_path, msg_width, msg_height)
            break

    def hbloop(self):
        while(True):
            self.mavlink_connection.mav.heartbeat_send(
                    mavutil.mavlink.MAV_TYPE_GENERIC,
                    mavutil.mavlink.MAV_AUTOPILOT_INVALID,
                    mavutil.mavlink.MAV_MODE_PREFLIGHT,
                    0,
                    mavutil.mavlink.MAV_STATE_ACTIVE)
            sleep(1)

    def mavloop(self):
        while(True):
            msg = self.mavlink_connection.recv_match(blocking=False)
            if not msg:
                continue

            msg_type = msg.get_type()
            if msg_type == "VIDEO_STREAM_GET":
                print("Message received: " + str(msg))
                self.handle_video_stream_get(msg)
            elif msg_type == "SET_VIDEO_STREAM_SETTINGS":
                print("Message received: " + str(msg))
                self.handle_set_video_stream_settings(msg)

            sleep(0.01)

def parse_args():
    parser = ArgumentParser(description=('A daemon that allows camera configuration handling'
        'via D-Bus methods and exports RTSP streams using GStreamer.'))
    parser.add_argument('-d', '--device', default='udp::', help='Serial, UDP, TCP or file mavlink connection.') #support multiple interfaces
    parser.add_argument('-t', '--target', default='udpbcast::14550', help='Target device (may be broadcast address when using udp).')
    parser.add_argument('-b', '--baud', default='115200', help='Baud rate.')
    parser.add_argument('-s', '--system_id', default=8, help='System ID.')

    return parser.parse_args()

if __name__ == "__main__":
    #TODO: Test serial, tcp and file support. And mavlink-route supporte

    args = parse_args()

    Gst.init(None)

    stream_manager = StreamManager()
    stream_manager.StartStreams()
    mavlink_manager = MavlinkManager(stream_manager)
    mavlink_manager.Start()

    GObject.MainLoop().run()
