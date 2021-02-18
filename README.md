# THIS REPOSITORY HAS BEEN DEPRECATED

# Dronecode Camera Manager

[![Build Status](https://travis-ci.org/Dronecode/camera-manager.svg?branch=master)](https://travis-ci.org/intel/camera-streaming-daemon)
<a href="https://scan.coverity.com/projects/01org-camera-streaming-daemon">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/12056/badge.svg"/>
</a>

The [Dronecode Camera Manager](https://camera-manager.dronecode.org/en/) (DCM) is an extensible Linux camera server for interfacing cameras with the Dronecode platform. 

It provides a [MAVLink Camera Protocol](https://mavlink.io/en/protocol/camera.html) compatible API for video and image capture etc., and an RTSP service for advertising and sharing video streams. The server can connect to and manage multiple cameras, and has been designed so that it can be extended to support new camera types and protocols when needed. 

> **Tip** The DCM is the easiest way for Camera OEMs to interface with the Dronecode platform. Many cameras will just work "out of the box". At most OEMs will need to implement a camera integration layer.

Full instructions for [building](https://camera-manager.dronecode.org/en/getting_started/), [using](https://camera-manager.dronecode.org/en/guide/overview.html), [extending](https://camera-manager.dronecode.org/en/guide/extending.html) and [contributing](https://camera-manager.dronecode.org/en/contribute/) to the camera manager can be found in the [guide](https://camera-manager.dronecode.org/en/).
