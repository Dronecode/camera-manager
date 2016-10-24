# Camera Streaming Daemon #

A daemon that allows camera configuration handling via D-Bus methods and exports RTSP streams using GStreamer.

## Main Requirements ##

    * Python 3
    * GStreamer 1.0 Python Package
    * D-Bus Python Package

## Run ##

Run the script:

    ./stream.py

## Interaction ##

Once the script is running, D-Bus methods can be called in order to control it.

For each camera an RTSP stream will be exported:
    e.g. rtsp://my.ip:port/stream0
         rtsp://my.ip:port/stream1
         rtsp://my.ip:port/stream2

The examples below show how to simply interact with these D-Bus methods:

Get available streams:

    busctl --system call org.ardupilot.camera.stream /org/ardupilot/StreamManager org.ardupilot.StreamManager GetStreams

Remove stream:

    busctl --system call org.ardupilot.camera.stream /org/ardupilot/StreamManager org.ardupilot.StreamManager RemoveStream s stream0

Add stream:

    busctl --system call org.ardupilot.camera.stream /org/ardupilot/StreamManager org.ardupilot.StreamManager AddStream sss /dev/video0 MJPG stream0

Get stream complete URI:

    busctl --system call org.ardupilot.camera.stream /org/ardupilot/stream/stream0 org.ardupilot.camera.stream GetURI

Get camera available formats:

    busctl --system call org.ardupilot.camera.stream /org/ardupilot/stream/stream0 org.ardupilot.camera GetFormats

Set stream format:

    busctl --system call org.ardupilot.camera.stream /org/ardupilot/stream/stream0 org.ardupilot.camera.stream SetFormat s YUYV

Get the stream on the client:

    gst-launch-1.0 -vvv rtspsrc location=rtsp://ip:port/stream0 ! rtpjpegdepay ! jpegdec ! autovideosink sync=false
