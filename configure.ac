AC_INIT([Logs],
        [0.1],
        [davidk@gnome.org],
        [gnome-logs],
        [https://wiki.gnome.org/Apps/Logs])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/gl-main.c])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 -Wall -Wno-portability check-news dist-xz no-define no-dist-gzip subdir-objects])
AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

dnl Check for programs
AC_PROG_CC
AM_PROG_CC_C_O

GNOME_COMPILE_WARNINGS([maximum])

PKG_PROG_PKG_CONFIG([0.24])

AC_MSG_CHECKING([for glib-mkenums script])
GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
AS_IF([test "x$GLIB_MKENUMS" = "x"],
      [AC_MSG_ERROR([glib-mkenums not listed in glib-2.0 pkg-config file])],
      [AC_SUBST([GLIB_MKENUMS])
       AC_MSG_RESULT([$GLIB_MKENUMS])])

AC_SUBST([GLIB_COMPILE_RESOURCES], [`$PKG_CONFIG --variable glib_compile_resources gio-2.0`])

dnl Internationalization
IT_PROG_INTLTOOL([0.50.0])
AC_SUBST([GETTEXT_PACKAGE], [AC_PACKAGE_TARNAME])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [gettext package name])

dnl Libraries
GTK_REQUIRED="gtk+-3.0 >= 3.9.6"
SYSTEMD_JOURNAL_REQUIRED="libsystemd-journal"
PKG_CHECK_MODULES([LOGS], [$GTK_REQUIRED $SYSTEMD_JOURNAL_REQUIRED])

AC_CONFIG_FILES([
Makefile
po/Makefile.in
])
AC_OUTPUT
