ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

bin_PROGRAMS = gnome-logs

gnome_logs_CPPFLAGS = \
	-I$(top_builddir)

gnome_logs_CFLAGS = \
	$(LOGS_CFLAGS)

gnome_logs_LDADD = \
	$(LOGS_LIBS)

gnome_logs_SOURCES = \
	src/gl-main.c

dist_noinst_SCRIPTS = \
	autogen.sh

MAINTAINERCLEANFILES = \
	build-aux/compile \
	build-aux/config.guess \
	build-aux/config.sub \
	build-aux/depcomp \
	build-aux/install-sh \
	build-aux/missing \
	aclocal.m4 \
	config.h.in \
	mkinstalldirs
