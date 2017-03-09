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
	-I$(top_srcdir) \
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
	${AVAHI_CFLAGS} \
	${GST_CFLAGS}

AM_CXXFLAGS = \
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
	${AVAHI_CFLAGS} \
	${GST_CFLAGS}

AM_LDFLAGS = \
	-Wl,--as-needed \
	-Wl,--no-undefined \
	-Wl,--gc-sections

base_files = \
	avahi_publisher.cpp \
	avahi_publisher.h \
	glib_mainloop.cpp \
	glib_mainloop.h \
	gstreamer_pipeline_builder.cpp \
	gstreamer_pipeline_builder.h \
	log.c \
	log.h \
	macro.h \
	mainloop.cpp \
	mainloop.h \
	rtsp_server.cpp \
	rtsp_server.h \
	stream.cpp \
	stream.h \
	stream_v4l2.cpp \
	stream_v4l2.h \
	stream_manager.cpp \
	stream_manager.h

bin_PROGRAMS += camera-streaming-daemon camera-client camera-sample-custom

if HAVE_REALSENSE
bin_PROGRAMS += camera-sample-realsense
endif

camera_streaming_daemon_SOURCES = \
	${base_files} \
	main.cpp

camera_sample_custom_SOURCES = \
	${base_files} \
	samples/main_sample_custom.cpp \
	samples/stream_custom.cpp \
	samples/stream_custom.h

camera_sample_realsense_SOURCES = \
	${base_files} \
	samples/main_sample_realsense.cpp \
	samples/stream_realsense.cpp \
	samples/stream_realsense.h

camera_client_SOURCES = \
	camera_client_main.cpp \
	log.c \
	log.h \
	glib_mainloop.cpp \
	glib_mainloop.h \
	mainloop.cpp \
	mainloop.h

camera_streaming_daemon_LDADD = $(GLIB_LIBS) $(AVAHI_LIBS) $(GST_LIBS)
camera_sample_custom_LDADD = ${camera_streaming_daemon_LDADD}
camera_sample_realsense_LDADD = ${camera_streaming_daemon_LDADD}
camera_sample_realsense_CXXFLAGS = ${AM_CXXFLAGS} -lrealsense
camera_client_LDADD = $(GLIB_LIBS) $(AVAHI_LIBS)

# ------------------------------------------------------------------------------
# coverity
# ------------------------------------------------------------------------------

#TODO
