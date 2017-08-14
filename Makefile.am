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
samples_deps =

clean-local:
	rm -rf $(top_builddir)/include/mavlink
	rm -rf $(EXTRA_PROGRAMS)

#if SYSTEMD
systemdsystemunitdir = @SYSTEMD_SYSTEMUNITDIR@
systemdsystemunit_DATA = csd.service
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
	src/util.h

if HAVE_REALSENSE
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

bin_PROGRAMS += csd

SED_PROCESS = $(AM_V_GEN) $(MKDIR_P) $(dir $@) && \
	$(SED) -e 's,@bindir\@,$(bindir),g' \
	< $< > $@

%.service: %.service.in Makefile
	$(SED_PROCESS)


csd_SOURCES = \
	${BASE_FILES} \
	src/main.cpp

csd_LDADD = $(GLIB_LIBS) $(GST_LIBS)

CLEANFILES += csd.service

#Samples
EXTRA_PROGRAMS = samples/camera-sample-custom

samples_camera_sample_custom_LDADD = ${csd_LDADD}

samples_camera_sample_custom_SOURCES = \
	$(BASE_FILES) \
	src/main.cpp \
	samples/stream_builder_custom.h \
	samples/stream_builder_custom.cpp \
	samples/stream_custom.cpp \
	samples/stream_custom.h

if !DISABLE_AVAHI
EXTRA_PROGRAMS += samples/camera-sample-client
BASE_FILES += \
	src/avahi_publisher.cpp \
	src/avahi_publisher.h

AM_CXXFLAGS += ${AVAHI_CFLAGS}
AM_CXXFLAGS += ${AVAHI_CFLAGS}
csd_LDADD += $(AVAHI_LIBS)

samples_camera_sample_client_SOURCES = \
	samples/main_sample_client.cpp \
	src/log.cpp \
	src/log.h \
	src/glib_mainloop.cpp \
	src/glib_mainloop.h \
	src/mainloop.cpp \
	src/mainloop.h

samples_camera_sample_client_LDADD = $(GLIB_LIBS) $(AVAHI_LIBS)
endif

if ENABLE_MAVLINK

EXTRA_PROGRAMS += samples/camera-sample-mavlink-client

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
	src/mavlink_server.h \
	src/CameraParameters.cpp \
	src/CameraParameters.h \
	src/CameraComponent.h

samples_camera_sample_mavlink_client_SOURCES = \
	samples/main_sample_mavlink_client.cpp \
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

samples_camera_sample_mavlink_client_LDADD = $(GLIB_LIBS)
if !DISABLE_AVAHI
samples_camera_sample_mavlink_client_LDADD += $(AVAHI_LIBS)
endif

samples_deps = samples_camera_sample_mavlink_client_LDADD
endif

.PHONY: samples
samples: $(samples_deps)
	$(MAKE) $(EXTRA_PROGRAMS)

# ------------------------------------------------------------------------------
# coverity
# ------------------------------------------------------------------------------

#TODO
