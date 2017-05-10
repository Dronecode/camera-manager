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

BUILT_SOURCES = include/mavlink/ardupilotmega/mavlink.h

clean-local:
	rm -rf $(top_builddir)/include/mavlink

include/mavlink/ardupilotmega/mavlink.h: modules/mavlink/pymavlink/tools/mavgen.py modules/mavlink/message_definitions/v1.0/ardupilotmega.xml
	$(AM_V_GEN)$(PYTHON) $(srcdir)/modules/mavlink/pymavlink/tools/mavgen.py \
		-o include/mavlink \
		--lang C \
		--wire-protocol 2.0 \
		$(srcdir)/modules/mavlink/message_definitions/v1.0/ardupilotmega.xml
#if SYSTEMD
systemdsystemunitdir = @SYSTEMD_SYSTEMUNITDIR@
systemdsystemunit_DATA = csd.service
#endif


AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I$(top_builddir)/include/mavlink \
	-I$(top_builddir)/include/mavlink/ardupilotmega \
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
	src/mavlink_server.cpp \
	src/mavlink_server.h \
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

bin_PROGRAMS += csd

SED_PROCESS = $(AM_V_GEN) $(MKDIR_P) $(dir $@) && \
	$(SED) -e 's,@bindir\@,$(bindir),g' \
	< $< > $@

%.service: %.service.in Makefile
	$(SED_PROCESS)


csd_SOURCES = \
	${base_files} \
	src/main.cpp

csd_LDADD = $(GLIB_LIBS) $(AVAHI_LIBS) $(GST_LIBS)

CLEANFILES += csd.service

.PHONY: samples
samples:
	@$(MAKE) -C samples


# ------------------------------------------------------------------------------
# coverity
# ------------------------------------------------------------------------------

#TODO
