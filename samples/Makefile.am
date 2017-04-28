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

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-DSYSCONFDIR=\""$(sysconfdir)"\"

AM_CFLAGS = \
	-pipe \
	-Wall \
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
	${AVAHI_CFLAGS} \
	${GST_CFLAGS}

AM_CXXFLAGS = \
	-I$(top_srcdir)/src \
	-pipe \
	-Wall \
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
	${AVAHI_CFLAGS} \
	${GST_CFLAGS}

AM_LDFLAGS = \
	-Wl,--as-needed \
	-Wl,--no-undefined \
	-Wl,--gc-sections

base_files = \
	../src/avahi_publisher.cpp \
	../src/avahi_publisher.h \
	../src/conf_file.h \
	../src/conf_file.cpp \
	../src/glib_mainloop.cpp \
	../src/glib_mainloop.h \
	../src/gstreamer_pipeline_builder.cpp \
	../src/gstreamer_pipeline_builder.h \
	../src/log.cpp \
	../src/log.h \
	../src/macro.h \
	../src/mainloop.cpp \
	../src/mainloop.h \
	../src/pollable.cpp \
	../src/pollable.h \
	../src/rtsp_server.cpp \
	../src/rtsp_server.h \
	../src/socket.cpp \
	../src/socket.h \
	../src/stream.cpp \
	../src/stream.h \
	../src/stream_builder.cpp \
	../src/stream_builder.h \
	../src/stream_builder_v4l2.cpp \
	../src/stream_builder_v4l2.h \
	../src/stream_v4l2.cpp \
	../src/stream_v4l2.h \
	../src/stream_manager.cpp \
	../src/stream_manager.h \
	../src/util.c \
	../src/util.h


bin_PROGRAMS += camera-sample-client camera-sample-custom

if HAVE_REALSENSE
bin_PROGRAMS += camera-sample-realsense
endif

camera_sample_custom_SOURCES = \
	${base_files} \
	../src/main.cpp \
	stream_builder_custom.h \
	stream_builder_custom.cpp \
	stream_custom.cpp \
	stream_custom.h

camera_sample_realsense_SOURCES = \
	${base_files} \
	../src/main.cpp \
	stream_builder_realsense.h \
	stream_builder_realsense.cpp \
	stream_realsense.cpp \
	stream_realsense.h

camera_sample_client_SOURCES = \
	main_sample_client.cpp \
	../src/log.cpp \
	../src/log.h \
	../src/glib_mainloop.cpp \
	../src/glib_mainloop.h \
	../src/mainloop.cpp \
	../src/mainloop.h

camera_streaming_daemon_LDADD = $(GLIB_LIBS) $(AVAHI_LIBS) $(GST_LIBS)
camera_sample_custom_LDADD = ${camera_streaming_daemon_LDADD}
camera_sample_realsense_LDADD = ${camera_streaming_daemon_LDADD}
camera_sample_client_LDADD = $(GLIB_LIBS) $(AVAHI_LIBS)
