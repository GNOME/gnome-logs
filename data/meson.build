subdir('icons')

add_global_arguments('-DGL_BINDIR="@0@"'.format(gl_bindir), language : 'c')

gettext_package = gl_name

desktop_file = i18n.merge_file(
    input : 'org.gnome.Logs.desktop.in',
    output : 'org.gnome.Logs.desktop',
    type : 'desktop',
    po_dir : po_dir,
    install : true,
    install_dir : join_paths(gl_datadir, 'applications')
)
desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
  test(
    'validate-desktop',
    desktop_file_validate,
    args: [
      desktop_file,
    ],
    depends: desktop_file,
  )
endif

appdata_file = i18n.merge_file(
    input : 'org.gnome.Logs.appdata.xml.in',
    output : 'org.gnome.Logs.appdata.xml',
    po_dir : po_dir,
    install : true,
    install_dir : join_paths(gl_datadir, 'metainfo')
)
appstreamcli = find_program('appstreamcli', required: false)
if appstreamcli.found()
  test(
    'appstreamcli-validate', appstreamcli,
    args: [
      'validate', '--no-net', '--explain', appdata_file,
    ],
    depends: appdata_file,
  )
endif

service_conf = configuration_data()
service_conf.set('bindir', gl_bindir)

service = 'org.gnome.Logs.service'

configure_file(
  input: service + '.in',
  output: service,
  install: true,
  install_dir: join_paths(gl_datadir, 'dbus-1', 'services'),
  configuration: service_conf
)

resource_data = files(
    'gl-categorylist.ui',
    'gl-eventviewlist.ui',
    'gl-eventtoolbar.ui',
    'gl-eventviewdetail.ui',
    'gl-searchpopover.ui',
    'gl-window.ui',
    'help-overlay.ui'
)

gresource = files('org.gnome.Logs.gresource.xml')

# GSettings schemas
# https://github.com/mesonbuild/meson/issues/1687
custom_target('gsettings-enums',
  input: '../src/gl-application.h',
  output: 'org.gnome.Logs.enums.xml',
  install: true,
  install_dir: join_paths(gl_datadir, 'glib-2.0', 'schemas'),
  capture: true,
  command: ['glib-mkenums',
    '--comments', '<!-- @comment@ -->',
    '--fhead', '<schemalist>',
    '--vhead', '  <@type@ id="org.gnome.Logs.@EnumName@">',
    '--vprod', '    <value nick="@valuenick@" value="@valuenum@"/>',
    '--vtail', '  </@type@>',
    '--ftail', '</schemalist>',
    '@INPUT@'
  ]
)
install_data(
    'org.gnome.Logs.gschema.xml',
    install_dir : join_paths(gl_datadir, 'glib-2.0', 'schemas')
)
glib_compile_schemas = find_program('glib-compile-schemas', required: true)
if glib_compile_schemas.found()
  test(
    'validate-gschema', glib_compile_schemas,
    args: [
      '--strict', '--dry-run', meson.current_build_dir()
    ],
  )
endif

version_conf = configuration_data()
version_conf.set('PACKAGE_VERSION', gl_version)

configure_file(
    input : 'version.xml.in',
    output : 'version.xml',
    configuration: version_conf,
)

if logs_enable_man
    custom_target('gl-manfile',
        input : 'gnome-logs.xml',
        output : 'gnome-logs.1',
        install : true,
        install_dir : gl_mandir,
        command : [
            xsltproc,
            '--nonet',
            '--path', join_paths(meson.source_root(), 'data') + ':' + 'data',
            '--output', 'data/',
            'http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl',
            '@INPUT@'
        ]
    )
endif
