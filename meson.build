project('gnome-logs', 'c', version : '49.0', default_options : ['warning_level=1'], meson_version: '>= 0.59.0')

gl_name = meson.project_name()
gl_version = meson.project_version()

gl_prefix = get_option('prefix')
gl_bindir = join_paths(gl_prefix, get_option('bindir'))
gl_libexecdir = join_paths(gl_prefix, get_option('libexecdir'))
gl_localedir = join_paths(gl_prefix, get_option('localedir'))
gl_datadir = join_paths(gl_prefix, get_option('datadir'))
gl_mandir = join_paths(gl_prefix, get_option('mandir'), 'man1')
gl_pkgdatadir = join_paths(gl_datadir, gl_name)

# warning cflags
warning_cflags = [
    '-Wstrict-prototypes',
    '-Wnested-externs',
    '-Werror=missing-prototypes',
    '-Werror=implicit-function-declaration',
    '-Werror=pointer-arith',
    '-Werror=init-self',
    '-Werror=format-security',
    '-Werror=format=2',
    '-Werror=missing-include-dirs',
    '-Werror=return-type'
]
c_compiler = meson.get_compiler('c')
supported_warning_cflags = c_compiler.get_supported_arguments(warning_cflags)
add_global_arguments(supported_warning_cflags, language : 'c')

config_h = configuration_data()

# defines
set_defines = [
  # package
  ['PACKAGE', gl_name],
  ['PACKAGE_BUGREPORT', 'https://gitlab.gnome.org/GNOME/gnome-logs/-/issues'],
  ['PACKAGE_NAME', 'Logs'],
  ['PACKAGE_STRING', '@0@ @1@'.format(gl_name, gl_version)],
  ['PACKAGE_TARNAME', gl_name],
  ['PACKAGE_URL', 'https://apps.gnome.org/Logs/'],
  ['PACKAGE_VERSION', gl_version],
  ['VERSION', gl_version],
  # i18n
  ['GETTEXT_PACKAGE', gl_name],
]

foreach define: set_defines
  config_h.set_quoted(define[0], define[1])
endforeach
configure_file(output : 'config.h',
               configuration : config_h)

# Dependencies
gsettings_desktop_schemas = dependency('gsettings-desktop-schemas', required : false)
if not gsettings_desktop_schemas.found()
    message('gsettings-desktop-schemas is required at runtime')
endif

gl_deps = [
    dependency('gio-unix-2.0', version : '>=2.43.90'),
    dependency('gtk4', version : '>=4.10.0'),
    dependency('libadwaita-1', version : '>=1.4.alpha'),
    dependency('libsystemd')
]

# Manpage
have_xsltproc = false
have_manutils = false
if get_option('man')
    # TODO: check xsl stylesheets as well#

    xsltproc = find_program('xsltproc', required : false)
    if xsltproc.found()
        have_xsltproc = true
    else
        have_xsltproc = false
    endif
endif

if have_xsltproc
    have_manutils = true
else
    if get_option('man')
        error('manpage generation requested but required utilities were not found')
        have_manutils = false
    endif
endif

if have_manutils
    logs_enable_man = true
else
    logs_enable_man = false
endif

# i18n
gnome = import('gnome')
i18n = import('i18n')

data_dir = join_paths(meson.source_root(), 'data')
po_dir = join_paths(meson.source_root(), 'po')

top_inc = include_directories('.')
src_inc = include_directories('src')

subdir('data')
subdir('src')
subdir('po')
subdir('help')
subdir('tests')

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
  update_desktop_database: true,
)
