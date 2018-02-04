subdir('icons')

add_global_arguments('-DGL_BINDIR="@0@"'.format(gl_bindir), language : 'c')

gettext_package = gl_name

i18n.merge_file(
    input : 'org.gnome.Logs.desktop.in',
    output : 'org.gnome.Logs.desktop',
    type : 'desktop',
    po_dir : 'po',
    install : true,
    install_dir : join_paths(gl_datadir, 'applications')
)

i18n.merge_file(
    input : 'org.gnome.Logs.appdata.xml.in',
    output : 'org.gnome.Logs.appdata.xml',
    po_dir : 'po',
    install : true,
    install_dir : join_paths(gl_datadir, 'appdata')
)

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
    'help-overlay.ui',
    'menus.ui'
)

gresource = files('org.gnome.Logs.gresource.xml')

web_files = files(
  'gl-style.css'
)

install_data(
  web_files,
  install_dir: gl_pkgdatadir
)

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

configure_file(
    input : 'version.xml.in',
    output : 'version.xml',
    command : ['cp', '@INPUT@', '@OUTPUT@']
)

if logs_enable_man
    custom_target('gl-manfile',
        input : 'gnome-logs.xml',
        output : 'gnoem-logs.1',
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