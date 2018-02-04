project('gnome-logs', 'c', version : '3.27.1')

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
    '-Wall',
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
  ['PACKAGE_BUGREPORT', 'http://bugzilla.gnome.org/enter_bug.cgi?product=' + gl_name],
  ['PACKAGE_NAME', gl_name],
  ['PACKAGE_STRING', '@0@ @1@'.format(gl_name, gl_version)],
  ['PACKAGE_TARNAME', gl_name],
  ['PACKAGE_URL', 'https://wiki.gnome.org/Apps/Logs'],
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
dependency('pkg-config', version : '>= 0.24')

message('checking for glib-mkenums script...')
glib_mkenums = run_command('pkg-config', ['--variable=glib_mkenums', 'glib-2.0'])
if not glib_mkenums.stdout().contains('glib-mkenums')
    error('glib-mkenums not listed in glib-2.0 pkg-config file')
else
    message(glib_mkenums.stdout().strip())
endif

gsettings_desktop_schemas = dependency('gsettings-desktop-schemas', required : false)
if not gsettings_desktop_schemas.found()
    message('gsettings-desktop-schemas is required at runtime')
endif

gl_deps = [
    dependency('gio-unix-2.0', version : '>=2.43.90'),
    dependency('gtk+-3.0', version : '>=3.22.0'),
    dependency('libsystemd')
]

# Testing utilities
have_desktop_file_validate = false
if get_option('tests')
    desktop_file_validate = find_program('desktop-file-validate', required : false)
    if desktop_file_validate.found()
        have_desktop_file_validate = true
    else
        have_desktop_file_validate = false
    endif

    py_module = 'dogtail'
    message('checking for python module ' + py_module + '...')
    result = run_command('echo', ['"import ' + py_module + '"| python - &>/dev/null'])
    if result.returncode() != 0
        message('not found')
        have_dogtail = false
    else
        message('found')
        have_dogtail = true
    endif
else
    have_desktop_file_validate = false
    have_dogtail = false
endif

if have_desktop_file_validate == true and have_dogtail == true
    testing_utilities = true
else
    testing_utilities = false
    if get_option('tests')
        error('tests were requested but the required utilities were not found')
    endif
endif

if testing_utilities
    logs_enable_tests = true
else
    logs_enable_tests = false
endif

# Manpage
have_manutils = false
if get_option('man')
    # TODO: check xsl stylesheets as well#

    xsltproc = find_program('xsltproc', required : false)
    if xsltproc.found()
        have_xsltproc = true
    else
        have_xsltproc = false
    endif

    if have_xsltproc
        have_manutils = true
    else
        if not get_option('disable-man')
            error('manpage generation requested but required utilities were not found')
            have_manutils = false
        endif
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

meson.add_install_script('meson_post_install.py')