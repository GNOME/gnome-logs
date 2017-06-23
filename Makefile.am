ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

AM_DISTCHECK_CONFIGURE_FLAGS = \
	--enable-appstream-util \
	--enable-tests

SUBDIRS = help po

bin_PROGRAMS = gnome-logs

BUILT_SOURCES = \
	$(nodist_gnome_logs_SOURCES)

gnome_logs_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\" \
	-I$(top_builddir) \
	-I$(top_builddir)/src \
	$(WARN_CFLAGS)

gnome_logs_CFLAGS = \
	$(LOGS_CFLAGS)

gnome_logs_LDADD = \
	$(LOGS_LIBS)

gnome_logs_SOURCES = \
	src/gl-application.c \
	src/gl-categorylist.c \
	src/gl-eventtoolbar.c \
	src/gl-eventviewdetail.c \
	src/gl-eventviewlist.c \
	src/gl-searchpopover.c \
	src/gl-eventviewrow.c \
	src/gl-journal.c \
	src/gl-journal-model.c \
	src/gl-main.c \
	src/gl-util.c \
	src/gl-window.c

nodist_gnome_logs_SOURCES = \
	src/gl-enums.c \
	src/gl-resource.c

enum_data = \
	src/gl-enums.c \
	src/gl-enums.h

gnome_logs_enum_headers = \
	$(srcdir)/src/gl-categorylist.h \
	$(srcdir)/src/gl-eventtoolbar.h \
	$(srcdir)/src/gl-eventviewrow.h \
	$(srcdir)/src/gl-util.h \
	$(srcdir)/src/gl-searchpopover.h \
	$(srcdir)/src/gl-journal-model.h

noinst_gnome_logs_headers = \
	src/gl-application.h \
	src/gl-categorylist.h \
	src/gl-eventtoolbar.h \
	src/gl-eventviewdetail.h \
	src/gl-eventviewlist.h \
	src/gl-searchpopover.h \
	src/gl-eventviewrow.h \
	src/gl-journal.h \
	src/gl-journal-model.h \
	src/gl-util.h \
	src/gl-window.h

nodist_gnome_logs_headers = \
	src/gl-enums.h \
	src/gl-resource.h

%/.dstamp:
	$(AM_V_at)$(MKDIR_P) $(@D)
	$(AM_V_at)touch $@

@APPSTREAM_XML_RULES@
@INTLTOOL_XML_RULE@
appstream_in_files = data/org.gnome.Logs.appdata.xml.in
$(appstream_in_files:.appdata.xml.in=.appdata.xml): data/.dstamp
appstream_XML = $(appstream_in_files:.appdata.xml.in=.appdata.xml)

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = data/org.gnome.Logs.desktop.in
$(desktop_in_files:.desktop.in=.desktop): data/.dstamp
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

data/org.gnome.Logs.service: data/org.gnome.Logs.service.in
	$(AM_V_GEN)$(SED) -e "s|[@]bindir[@]|$(bindir)|" $< > $@

servicedir = $(datadir)/dbus-1/services
service_in_files = data/org.gnome.Logs.service.in
service_DATA = $(service_in_files:.service.in=.service)

gsettings_ENUM_FILES = src/gl-application.h
gsettings_ENUM_NAMESPACE = org.gnome.Logs
gsettings_SCHEMAS = data/org.gnome.Logs.gschema.xml
data/org.gnome.Logs.gschema.valid: data/.dstamp
@GSETTINGS_RULES@

if ENABLE_MAN
data/gnome-logs.1: data/gnome-logs.xml
	$(AM_V_GEN)xsltproc --nonet --path $(builddir)/data --output $(builddir)/data/ http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

nodist_man_MANS = data/gnome-logs.1
endif

if LOGS_ENABLE_TESTS
tests/basic.py: gnome-logs

test-desktop-file-validate.sh: Makefile
	$(AM_V_GEN)echo "#!/bin/sh -e" > $@; \
		echo "$(DESKTOP_FILE_VALIDATE) $(top_builddir)/$(desktop_DATA) || exit 1" >> $@; \
		chmod +x $@

check_SCRIPTS = \
	test-desktop-file-validate.sh

# GSettings schema must be compiled and gnome-logs installed in PATH before
# running the test.
installcheck-local:
	$(AM_V_at)env $(TESTS_ENVIRONMENT) $(top_srcdir)/tests/basic.py

# Tests:
TESTS_ENVIRONMENT = \
	LANG=C \
	TZ=UTC \
	MALLOC_CHECK_=2 \
	MALLOC_PERTURB_=$$(($${RANDOM:-256} % 256)) \
	G_SLICE=debug-blocks

tests_test_gl_util_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\" \
	-I$(top_builddir) \
	-I$(top_srcdir)/src \
	$(WARN_CFLAGS)

tests_test_gl_util_CFLAGS = \
	$(LOGS_CFLAGS)

tests_test_gl_util_LDADD = \
	$(LOGS_LIBS)

tests_test_gl_util_SOURCES = \
	tests/test-gl-util.c \
	src/gl-util.c

check_PROGRAMS = \
	tests/test-gl-util

TESTS = \
	$(check_PROGRAMS) \
	$(check_SCRIPTS)
endif

iconthemedir = $(datadir)/icons/hicolor

appicon16dir = $(iconthemedir)/16x16/apps
appicon22dir = $(iconthemedir)/22x22/apps
appicon24dir = $(iconthemedir)/24x24/apps
appicon32dir = $(iconthemedir)/32x32/apps
appicon48dir = $(iconthemedir)/48x48/apps
appicon256dir = $(iconthemedir)/256x256/apps
appicon512dir = $(iconthemedir)/512x512/apps
appiconsymbolicdir = $(iconthemedir)/symbolic/apps

dist_appicon16_DATA = data/icons/16x16/gnome-logs.png
dist_appicon22_DATA = data/icons/22x22/gnome-logs.png
dist_appicon24_DATA = data/icons/24x24/gnome-logs.png
dist_appicon32_DATA = data/icons/32x32/gnome-logs.png
dist_appicon48_DATA = data/icons/48x48/gnome-logs.png
dist_appicon256_DATA = data/icons/256x256/gnome-logs.png
dist_appicon512_DATA = data/icons/512x512/gnome-logs.png
dist_appiconsymbolic_DATA = data/icons/symbolic/gnome-logs-symbolic.svg

update_icon_cache = gtk-update-icon-cache --ignore-theme-index --force

install-data-hook: install-update-icon-cache
uninstall-hook: uninstall-update-icon-cache

install-update-icon-cache:
	$(AM_V_at)$(POST_INSTALL)
	test -n "$(DESTDIR)" || $(update_icon_cache) "$(iconthemedir)"

uninstall-update-icon-cache:
	$(AM_V_at)$(POST_UNINSTALL)
	test -n "$(DESTDIR)" || $(update_icon_cache) "$(iconthemedir)"

src/gl-enums.c: $(gnome_logs_enum_headers) src/gl-enums.h Makefile
	$(AM_V_GEN)$(GLIB_MKENUMS) \
		--fhead "#include \"gl-enums.h\"\n\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--fprod "#include \"@filename@\"\n" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n" \
		--vhead "    static GType type = 0;\n\n" \
		--vhead "    if (!type)\n    {\n" \
		--vhead "        static const G@Type@Value _@enum_name@_values[] = {" \
		--vprod "            { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "            { 0, NULL, NULL }\n        };\n\n" \
		--vtail "        type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n    }\n\n" \
		--vtail "  return type;\n}\n\n" \
		$(gnome_logs_enum_headers) > $@

src/gl-enums.h: $(gnome_logs_enum_headers) Makefile
	$(AM_V_GEN)$(GLIB_MKENUMS) \
		--fhead "#ifndef GL_ENUMS_H_\n" \
		--fhead "#define GL_ENUMS_H_\n\n" \
		--fhead "#include <glib-object.h>\n\n" \
		--fhead "G_BEGIN_DECLS\n\n" \
		--fprod "/* enumerations from @filename@ */\n" \
		--vhead "GType @enum_name@_get_type (void);\n" \
		--vhead "#define GL_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n" \
		--ftail "#endif /* GL_ENUMS_H_ */" \
		$(gnome_logs_enum_headers) > $@

noinst_resource_files = $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir=$(srcdir)/data $(srcdir)/data/org.gnome.Logs.gresource.xml)

src/gl-resource.c src/gl-resource.h: data/org.gnome.Logs.gresource.xml Makefile $(noinst_resource_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/data --generate --c-name gl $<

dist_noinst_DATA = \
	$(noinst_gnome_logs_headers) \
	$(noinst_resource_files) \
	$(appstream_in_files) \
	$(desktop_in_files) \
	$(service_in_files) \
	$(gsettings_SCHEMAS) \
	data/org.gnome.Logs.gresource.xml \
	data/gnome-logs.xml \
	data/icons/gnome-logs.svg

dist_noinst_SCRIPTS = \
	autogen.sh \
	tests/basic.py

dist-hook: git-changelog-hook

clean-local: clean-local-dstamp
clean-local-dstamp:
	-rm -f data/.dstamp

git-changelog-hook:
	$(AM_V_at)if $(top_srcdir)/build-aux/missing --run git \
		--git-dir=$(top_srcdir)/.git --work-tree=$(top_srcdir) log \
		--stat -M -C --name-status --no-color \
		| fmt --split-only >.ChangeLog.tmp; \
	then mv -f .ChangeLog.tmp "$(top_distdir)/ChangeLog"; \
	else rm -f .ChangeLog.tmp; exit 1; fi

CLEANFILES = \
	$(appstream_XML) \
	$(desktop_DATA) \
	$(enum_data) \
	$(nodist_man_MANS) \
	$(service_DATA) \
	src/gl-resource.c \
	src/gl-resource.h \
	test-desktop-file-validate.sh

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

.PHONY: clean-local-dstamp git-changelog-hook
.PHONY: install-update-icon-cache uninstall-update-icon-cache
