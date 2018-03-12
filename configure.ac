AC_INIT([Logs],
        [3.28.0],
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
AC_PROG_SED

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
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])
AC_SUBST([GETTEXT_PACKAGE], [AC_PACKAGE_TARNAME])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [gettext package name])

dnl GSettings schemas
PKG_CHECK_EXISTS([gsettings-desktop-schemas],
                 [],
                 [AC_MSG_WARN([gsettings-desktop-schemas is required at runtime])])

# Manpage, generated with xsltproc from DocBook XML.
AC_ARG_ENABLE([man],
  [AS_HELP_STRING([--disable-man], [Disable building the man page])])

AS_IF([test "x$enable_man" != "xno"],
  [AC_CHECK_PROG([have_xsltproc], [xsltproc], [yes], [no])
   JH_CHECK_XML_CATALOG([-//OASIS//DTD DocBook XML V4.3//EN],
     [DocBook XML DTD V4.3], [have_docbookdtd=yes], [have_docbookdtd=no])
   JH_CHECK_XML_CATALOG(
     [http://docbook.sourceforge.net/release/xsl/current/html/refentry.xsl],
     [DocBook XSL Stylesheets], [have_docbookxsl=yes], [have_docbookxsl=no])
   AS_IF([test "x$have_xsltproc" = "xyes" -a "x$have_docbookdtd" = "xyes" \
     -a "x$have_docbookxsl" = "xyes"],
     [have_manutils=yes],
     [AS_IF([test "x$enable_man" = "xyes"],
       [AC_MSG_ERROR(
         [manpage generation requested but required utilities were not found])])
      have_manutils=no])],
  [have_manutils=no])

AM_CONDITIONAL([ENABLE_MAN], [test "x$have_manutils" = "xyes"])

dnl AppData XML
APPSTREAM_XML

dnl Mallard Help
YELP_HELP_INIT

dnl Testing utilities.
AC_ARG_ENABLE([tests],
              [AS_HELP_STRING([--disable-tests], [Disable support for tests run during make check])])

AS_IF([test "x$enable_tests" != "xno"],
      [AC_PATH_PROG([DESKTOP_FILE_VALIDATE], [desktop-file-validate])
       AS_IF([test "x$ac_cv_path_DESKTOP_FILE_VALIDATE" != "xyes"],
             [have_desktop_file_validate=yes],
             [have_desktop_file_validate=no])
       AM_PATH_PYTHON([2.7])
       py_module=dogtail
       AC_MSG_CHECKING([for python module $py_module])
       echo "import $py_module" | python - &>/dev/null
       AS_IF([test $? != 0],
             [AC_MSG_RESULT([not found])
              have_dogtail=no],
             [AC_MSG_RESULT([found])
              have_dogtail=yes])],
      [have_desktop_file_validate=no
       have_dogtail=no])

AS_IF([test "x$have_desktop_file_validate" = "xyes" \
       -a "x$have_dogtail" = "xyes"],
      [testing_utilities=yes],
      [testing_utilities=no
       AS_IF([test "x$enable_tests" = "xyes"],
             [AC_MSG_ERROR([tests were requested but the required utilities were not found])])])

AM_CONDITIONAL([LOGS_ENABLE_TESTS], [test "x$testing_utilities" = "xyes"])

dnl Libraries
GIO_REQUIRED="gio-unix-2.0 >= 2.43.90"
GTK_REQUIRED="gtk+-3.0 >= 3.22.0"
SYSTEMD_REQUIRED="libsystemd"
PKG_CHECK_MODULES([LOGS], [$GIO_REQUIRED $GTK_REQUIRED $SYSTEMD_REQUIRED])

GLIB_GSETTINGS

AC_CONFIG_FILES([
data/version.xml
help/Makefile
Makefile
po/Makefile.in
])
AC_OUTPUT
