DISTCLEAN_LOCAL_HOOKS =
EXTRA_DIST =
CLEANFILES = $(BUILT_FILES)
noinst_LTLIBRARIES =
bin_PROGRAMS =
noinst_PROGRAMS =
noinst_SCRIPTS =
BUILT_FILES =
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory

GCC_COLORS ?= 'yes'
export GCC_COLORS

BUILT_SOURCES =
test_deps =

clean-local:
	rm -rf $(top_builddir)/include/mavlink
	rm -rf $(EXTRA_PROGRAMS)

#if SYSTEMD
systemdsystemunitdir = @SYSTEMD_SYSTEMUNITDIR@
systemdsystemunit_DATA = dronecode-camera-manager.service
#endif


AM_CPPFLAGS = \
	-include $(abs_top_builddir)/config.h \
	-DSYSCONFDIR=\""$(sysconfdir)"\"

AM_CFLAGS = \
	-pipe \
	-Wall \
	-W \
	-Wextra \
	-Wno-inline \
	-Wundef \
	-Wformat=2 \
	-Wlogical-op \
	-Wsign-compare \
	-Wformat-security \
	-Wmissing-include-dirs \
	-Wformat-nonliteral \
	-Wold-style-definition \
	-Wpointer-arith \
	-Winit-self \
	-Wdeclaration-after-statement \
	-Wfloat-equal \
	-Wmissing-prototypes \
	-Wstrict-prototypes \
	-Wredundant-decls \
	-Wmissing-declarations \
	-Wmissing-noreturn \
	-Wshadow \
	-Wendif-labels \
	-Wstrict-aliasing=3 \
	-Wwrite-strings \
	-Wno-long-long \
	-Wno-overlength-strings \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-Wno-unused-result \
	-Wnested-externs \
	-Wchar-subscripts \
	-Wtype-limits \
	-Wuninitialized \
	-fno-common \
	-fdiagnostics-show-option \
	-fvisibility=hidden \
	-ffunction-sections \
	-fdata-sections \
	${GST_CFLAGS}

AM_CXXFLAGS = \
	-I$(top_builddir)/src \
	-I$(srcdir)/ \
	-pipe \
	-Wall \
	-W \
	-Wextra \
	-Wno-inline \
	-Wundef \
	-Wformat=2 \
	-Wlogical-op \
	-Wsign-compare \
	-Wformat-security \
	-Wmissing-include-dirs \
	-Wformat-nonliteral \
	-Wpointer-arith \
	-Winit-self \
	-Wfloat-equal \
	-Wredundant-decls \
	-Wmissing-declarations \
	-Wmissing-noreturn \
	-Wshadow \
	-Wendif-labels \
	-Wstrict-aliasing=3 \
	-Wwrite-strings \
	-Wno-long-long \
	-Wno-overlength-strings \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-Wno-unused-result \
	-Wchar-subscripts \
	-Wtype-limits \
	-Wuninitialized \
	-fno-common \
	-fdiagnostics-show-option \
	-fvisibility=hidden \
	-ffunction-sections \
	-fdata-sections \
	${GST_CFLAGS}

AM_LDFLAGS = \
	-Wl,--as-needed \
	-Wl,--no-undefined \
	-Wl,--gc-sections

BASE_FILES = \
	src/conf_file.cpp \
	src/conf_file.h \
	src/glib_mainloop.cpp \
	src/glib_mainloop.h \
	src/gstreamer_pipeline_builder.cpp \
	src/gstreamer_pipeline_builder.h \
	src/log.cpp \
	src/log.h \
	src/macro.h \
	src/mainloop.cpp \
	src/mainloop.h \
	src/pollable.cpp \
	src/pollable.h \
	src/rtsp_server.cpp \
	src/rtsp_server.h \
	src/socket.cpp \
	src/socket.h \
	src/stream.cpp \
	src/stream.h \
	src/stream_builder.cpp \
	src/stream_builder.h \
	src/stream_builder_v4l2.cpp \
	src/stream_builder_v4l2.h \
	src/stream_v4l2.cpp \
	src/stream_v4l2.h \
	src/stream_manager.cpp \
	src/stream_manager.h \
	src/util.c \
	src/util.h \
        src/CameraParameters.cpp \
        src/CameraParameters.h \
        src/CameraComponent.cpp \
        src/CameraComponent.h \
        src/CameraServer.cpp \
        src/CameraServer.h \
        src/CameraDevice.h \
        src/ImageCapture.h \
        src/ImageCaptureGst.h \
        src/ImageCaptureGst.cpp \
        src/VideoStream.h \
        src/VideoStreamUdp.h \
        src/VideoStreamUdp.cpp \
        src/VideoCapture.h \
        src/VideoCaptureGst.h \
        src/VideoCaptureGst.cpp \
        src/PluginManager.h \
        src/PluginManager.cpp \
        src/PluginBase.h \
        plugins/V4l2Camera/v4l2_interface.cpp \
        plugins/V4l2Camera/v4l2_interface.h\
        plugins/V4l2Camera/PluginV4l2.h\
        plugins/V4l2Camera/PluginV4l2.cpp \
        plugins/V4l2Camera/CameraDeviceV4l2.cpp \
        plugins/V4l2Camera/CameraDeviceV4l2.h

if ENABLE_REALSENSE
BASE_FILES += \
	src/stream_builder_realsense.h \
	src/stream_builder_realsense.cpp \
	src/stream_realsense.cpp \
	src/stream_realsense.h
endif

if ENABLE_AERO
BASE_FILES += \
	src/stream_builder_aero_bottom.h \
	src/stream_builder_aero_bottom.cpp \
	src/stream_aero_bottom.cpp \
	src/stream_aero_bottom.h
endif

bin_PROGRAMS += dcm

SED_PROCESS = $(AM_V_GEN) $(MKDIR_P) $(dir $@) && \
	$(SED) -e 's,@bindir\@,$(bindir),g' \
	< $< > $@

%.service: %.service.in Makefile
	$(SED_PROCESS)


dcm_SOURCES = \
	${BASE_FILES} \
	src/main.cpp

dcm_LDADD = $(GLIB_LIBS) $(GST_LIBS) $(GZB_LIBS)

CLEANFILES += dronecode-camera-manager.service

#Samples
EXTRA_PROGRAMS = test/test-custom-stream

test_test_custom_stream_LDADD = ${dcm_LDADD}

test_test_custom_stream_SOURCES = \
	$(BASE_FILES) \
	src/main.cpp \
	test/stream_builder_custom.h \
	test/stream_builder_custom.cpp \
	test/stream_custom.cpp \
	test/stream_custom.h

if ENABLE_AVAHI
EXTRA_PROGRAMS += test/test-rtsp-udp-stream-discovery
BASE_FILES += \
	src/avahi_publisher.cpp \
	src/avahi_publisher.h

AM_CXXFLAGS += ${AVAHI_CFLAGS}
AM_CXXFLAGS += ${AVAHI_CFLAGS}
dcm_LDADD += $(AVAHI_LIBS)

test_test_rtsp_udp_stream_discovery_SOURCES = \
	test/test_rtsp_udp_stream_discovery.cpp \
	src/log.cpp \
	src/log.h \
	src/glib_mainloop.cpp \
	src/glib_mainloop.h \
	src/mainloop.cpp \
	src/mainloop.h

test_test_rtsp_udp_stream_discovery_LDADD = $(GLIB_LIBS) $(AVAHI_LIBS)
endif

if ENABLE_MAVLINK

EXTRA_PROGRAMS += test/test-camera-parameters

test_test_camera_parameters_SOURCES = \
        src/CameraParameters.cpp \
        src/CameraParameters.h \
        test/test_camera_parameters.cpp \
        src/log.cpp \
        src/log.h \
        src/util.c \
        src/util.h

test_test_camera_parameters_LDADD = $(GLIB_LIBS)

EXTRA_PROGRAMS += test/test-mavlink-protocol

BUILT_SOURCES += include/mavlink/ardupilotmega/mavlink.h
include/mavlink/ardupilotmega/mavlink.h: modules/mavlink/pymavlink/tools/mavgen.py modules/mavlink/message_definitions/v1.0/ardupilotmega.xml
	$(AM_V_GEN)$(PYTHON) $(srcdir)/modules/mavlink/pymavlink/tools/mavgen.py \
		-o include/mavlink \
		--lang C \
		--wire-protocol 2.0 \
		$(srcdir)/modules/mavlink/message_definitions/v1.0/ardupilotmega.xml

AM_CPPFLAGS += \
	-I$(abs_top_builddir)/include/mavlink \
	-I$(abs_top_builddir)/include/mavlink/ardupilotmega

BASE_FILES += \
	src/mavlink_server.cpp \
	src/mavlink_server.h

test_test_mavlink_protocol_SOURCES = \
        test/test_mavlink_protocol.cpp \
        src/log.cpp \
        src/log.h \
        src/glib_mainloop.cpp \
        src/glib_mainloop.h \
        src/mainloop.cpp \
        src/mainloop.h \
        src/pollable.cpp \
        src/pollable.h \
        src/socket.cpp \
        src/socket.h \
        src/util.c \
        src/util.h

test_test_mavlink_protocol_LDADD = $(GLIB_LIBS)
if ENABLE_AVAHI
test_test_mavlink_protocol_LDADD += $(AVAHI_LIBS)
endif

test_deps = test_test_mavlink_protocol_LDADD
endif

if ENABLE_GAZEBO

AM_CXXFLAGS += \
        ${GZB_CFLAGS}

BASE_FILES += \
	src/CameraDeviceGazebo.h \
	src/CameraDeviceGazebo.cpp
endif

.PHONY: test
test: $(test_deps)
	$(MAKE) $(EXTRA_PROGRAMS)

# ------------------------------------------------------------------------------
# coverity
# ------------------------------------------------------------------------------

#TODO
