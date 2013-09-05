ACLOCAL_AMFLAGS = -I m4 -I libgd ${ACLOCAL_FLAGS}

SUBDIRS = libgd po

bin_PROGRAMS = gnome-logs

BUILT_SOURCES = \
	$(nodist_gnome_logs_SOURCES)

gnome_logs_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\" \
	-I$(top_builddir) \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/libgd \
	$(WARN_CFLAGS)

gnome_logs_CFLAGS = \
	$(LOGS_CFLAGS)

gnome_logs_LDADD = \
	$(LOGS_LIBS) \
	$(top_builddir)/libgd/libgd.la

gnome_logs_SOURCES = \
	src/gl-application.c \
	src/gl-categorylist.c \
	src/gl-eventview.c \
	src/gl-main.c \
	src/gl-window.c

nodist_gnome_logs_SOURCES = \
	src/gl-enums.c \
	src/gl-resource.c

enum_data = \
	src/gl-enums.c \
	src/gl-enums.h

gnome_logs_enum_headers = \
	$(srcdir)/src/gl-eventview.h

noinst_gnome_logs_headers = \
	src/gl-application.h \
	src/gl-categorylist.h \
	src/gl-eventview.h \
	src/gl-window.h

nodist_gnome_logs_headers = \
	src/gl-enums.h \
	src/gl-resource.h

%/.dstamp:
	$(AM_V_at)$(MKDIR_P) $(@D)
	$(AM_V_at)touch $@

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = data/gnome-logs.desktop.in
$(desktop_in_files:.desktop.in=.desktop): data/.dstamp
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

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

noinst_resource_files = \
	data/appmenu.ui \
	data/gl-categorylist.ui \
	data/gl-window.ui

src/gl-resource.c src/gl-resource.h: data/gnome-logs.gresource.xml Makefile $(noinst_resource_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/data --generate --c-name gl $<

dist_noinst_DATA = \
	$(noinst_gnome_logs_headers) \
	$(noinst_resource_files) \
	$(desktop_in_files) \
	data/gnome-logs.gresource.xml

dist_noinst_SCRIPTS = \
	autogen.sh

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
	$(desktop_DATA) \
	$(enum_data) \
	src/gl-resource.c \
	src/gl-resource.h

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
