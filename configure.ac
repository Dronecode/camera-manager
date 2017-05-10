AC_PREREQ(2.64)
AC_INIT([camera-streaming-daemon],
    [1],
    [],
    [camera-streaming-daemon],
    [])

PKG_CHECK_MODULES(GLIB, [glib-2.0])
PKG_CHECK_MODULES(AVAHI, [avahi-client, avahi-core, avahi-glib])
PKG_CHECK_MODULES(GST, [gstreamer-rtsp-1.0, gstreamer-1.0, gstreamer-rtsp-server-1.0 gstreamer-app-1.0])

AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_AUX_DIR([build-aux])
AC_SEARCH_LIBS([rs_create_context], [realsense], [AM_CONDITIONAL([HAVE_REALSENSE], true)], [AM_CONDITIONAL([HAVE_REALSENSE], false)])

AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_PREFIX_DEFAULT([/usr])
PKG_PROG_PKG_CONFIG
AM_MAINTAINER_MODE([enable])
AM_INIT_AUTOMAKE([check-news foreign 1.11 silent-rules tar-pax no-dist-gzip dist-xz subdir-objects color-tests parallel-tests])
AM_SILENT_RULES([yes])

#####################################################################
# Program checks and configurations
#####################################################################

AC_PROG_CC_C99
AX_CXX_COMPILE_STDCXX([11], [ext], [mandatory])
AM_PATH_PYTHON([2.7])

#####################################################################

LT_INIT([disable-static pic-only])

#####################################################################
# Function and structure checks
#####################################################################

AC_MSG_CHECKING([whether _Static_assert() is supported])
AC_COMPILE_IFELSE(
    [AC_LANG_SOURCE([[_Static_assert(1, "Test");]])],
        [AC_DEFINE([HAVE_STATIC_ASSERT], [1], [Define if _Static_assert() is available])
     AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([whether _Noreturn is supported])
AC_COMPILE_IFELSE(
    [AC_LANG_SOURCE([[_Noreturn int foo(void) { exit(0); }]])],
        [AC_DEFINE([HAVE_NORETURN], [1], [Define if _Noreturn is available])
     AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])])


#####################################################################
# --with-
#####################################################################

AC_ARG_WITH([rootlibdir],
        AS_HELP_STRING([--with-rootlibdir=DIR], [rootfs directory to install shared libraries]),
        [], [with_rootlibdir=$libdir])
AC_ARG_WITH([systemdsystemunitdir],
			AC_HELP_STRING([--with-systemdsystemunitdir=DIR],
			[path to systemd system unit directory]),
					[path_systemunitdir=${withval}])
if (test "${enable_systemd}" != "no" && test -z "${path_systemunitdir}"); then
	AC_MSG_CHECKING([systemd system unit dir])
	path_systemunitdir="`$PKG_CONFIG --variable=systemdsystemunitdir systemd`"
	if (test -z "${path_systemunitdir}"); then
		AC_MSG_ERROR([systemd system unit directory is required])
	fi
	AC_MSG_RESULT([${path_systemunitdir}])
fi

AC_ARG_WITH([systemduserunitdir],
			AC_HELP_STRING([--with-systemduserunitdir=DIR],
			[path to systemd user unit directory]),
					[path_userunitdir=${withval}])
if (test "${enable_systemd}" != "no" && test -z "${path_userunitdir}"); then
	AC_MSG_CHECKING([systemd user unit dir])
	path_userunitdir="`$PKG_CONFIG --variable=systemduserunitdir systemd`"
	if (test -z "${path_userunitdir}"); then
		AC_MSG_ERROR([systemd user unit directory is required])
	fi
	AC_MSG_RESULT([${path_userunitdir}])
fi
AC_SUBST([rootlibdir], [$with_rootlibdir])

#####################################################################
# --enable-
#####################################################################
AC_ARG_ENABLE(systemd, AC_HELP_STRING([--disable-systemd],
		[disable systemd integration]), [enable_systemd=${enableval}])
AM_CONDITIONAL(SYSTEMD, test "${enable_systemd}" != "no")

#####################################################################
# Default CFLAGS and LDFLAGS
#####################################################################
DEFAULT_FLAGS=" -fstack-protector-all -fPIE -fPIC -O2 -D_FORTIFY_SOURCE=2 -pthread -Wformat -Wformat-security"
CFLAGS+=$DEFAULT_FLAGS
CXXFLAGS+=$DEFAULT_FLAGS
LDFLAGS+=" -z noexecstack -z relro -z now"

#####################################################################
# Generate files from *.in
#####################################################################

AC_CONFIG_FILES([
    Makefile
    samples/Makefile
])

#####################################################################
AC_SUBST(SYSTEMD_SYSTEMUNITDIR, [${path_systemunitdir}])

AC_SUBST(SYSTEMD_USERUNITDIR, [${path_userunitdir}])

AC_OUTPUT
AC_MSG_RESULT([
    $PACKAGE $VERSION
    ================

    prefix:            ${prefix}
    sysconfdir:        ${sysconfdir}
    libdir:            ${libdir}
    rootlibdir:        ${rootlibdir}
    includedir:        ${includedir}
    bindir:            ${bindir}

    C compiler:        ${CC}
    C++ compiler:      ${CXX}
    CFLAGS:            ${CFLAGS}
    CXXFLAGS:          ${CXXFLAGS}
    LDFLAGS:           ${LDFLAGS}
])
