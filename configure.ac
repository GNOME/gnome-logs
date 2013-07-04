AC_INIT([GNOME Logs],
        [0.1],
        [davidk@gnome.org],
        [gnome-logs],
        [https://wiki.gnome.org/Design/Apps/SystemLog])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/gl-main.c])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 -Wall dist-xz no-define no-dist-gzip subdir-objects])
AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

dnl Check for programs
AC_PROG_CC
AM_PROG_CC_C_O

LT_INIT

PKG_PROG_PKG_CONFIG([0.24])

dnl Libraries
PKG_CHECK_MODULES([LOGS], [gtk+-3.0 >= 3.9.6])

LIBGD_INIT([main-toolbar])

AC_CONFIG_FILES([
Makefile
libgd/Makefile
])
AC_OUTPUT
