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
	src/avahi_publisher.cpp \
	src/avahi_publisher.h \
	src/glib_mainloop.cpp \
	src/glib_mainloop.h \
	src/gstreamer_pipeline_builder.cpp \
	src/gstreamer_pipeline_builder.h \
	src/log.c \
	src/log.h \
	src/macro.h \
	src/mainloop.cpp \
	src/mainloop.h \
	src/rtsp_server.cpp \
	src/rtsp_server.h \
	src/stream.cpp \
	src/stream.h \
	src/stream_builder.cpp \
	src/stream_builder.h \
	src/stream_builder_v4l2.cpp \
	src/stream_builder_v4l2.h \
	src/stream_v4l2.cpp \
	src/stream_v4l2.h \
	src/stream_manager.cpp \
	src/stream_manager.h

bin_PROGRAMS += camera-streaming-daemon

camera_streaming_daemon_SOURCES = \
	${base_files} \
	src/main.cpp

camera_streaming_daemon_LDADD = $(GLIB_LIBS) $(AVAHI_LIBS) $(GST_LIBS)

.PHONY: samples
samples:
	@$(MAKE) -C samples


# ------------------------------------------------------------------------------
# coverity
# ------------------------------------------------------------------------------

#TODO
