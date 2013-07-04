ACLOCAL_AMFLAGS = -I m4 -I libgd ${ACLOCAL_FLAGS}

SUBDIRS = libgd

bin_PROGRAMS = gnome-logs

gnome_logs_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir)/libgd

gnome_logs_CFLAGS = \
	$(LOGS_CFLAGS)

gnome_logs_LDADD = \
	$(LOGS_LIBS) \
	$(top_builddir)/libgd/libgd.la

gnome_logs_SOURCES = \
	src/gl-main.c \
	src/gl-window.c

noinst_gnome_logs_headers = \
	src/gl-window.h

dist_noinst_DATA = \
	$(noinst_gnome_logs_headers)

dist_noinst_SCRIPTS = \
	autogen.sh

dist-hook: git-changelog-hook

git-changelog-hook:
	$(AM_V_at)if $(top_srcdir)/build-aux/missing --run git \
		--git-dir=$(top_srcdir)/.git --work-tree=$(top_srcdir) log \
		--stat -M -C --name-status --no-color \
		| fmt --split-only >.ChangeLog.tmp; \
	then mv -f .ChangeLog.tmp "$(top_distdir)/ChangeLog"; \
	else rm -f .ChangeLog.tmp; exit 1; fi

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

.PHONY: git-changelog-hook
