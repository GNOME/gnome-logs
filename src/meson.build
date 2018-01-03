add_global_arguments('-DLOCALEDIR="@0@"'.format(gl_localedir), language : 'c')

sources = files(
	'gl-application.c',
	'gl-categorylist.c',
	'gl-eventtoolbar.c',
	'gl-eventviewdetail.c',
	'gl-eventviewlist.c',
	'gl-searchpopover.c',
	'gl-eventviewrow.c',
	'gl-journal.c',
	'gl-journal-model.c',
	'gl-main.c',
	'gl-util.c',
	'gl-window.c'
)

enum_headers = files(
    'gl-categorylist.h',
    'gl-eventtoolbar.h',
    'gl-eventviewrow.h',
    'gl-journal-model.h',
    'gl-searchpopover.h',
    'gl-util.h'
)

sources += gnome.compile_resources(
  'gl-resources',
  gresource,
  c_name: 'gl',
  source_dir: data_dir,
  dependencies: resource_data,
  export: true
)

enum = 'gl-enums'
sources += gnome.mkenums_simple(
    enum,
    sources : enum_headers
)

executable(
    gl_name,
    sources,
    include_directories : [top_inc, src_inc],
    dependencies : gl_deps,
    install : true,
    install_dir : gl_bindir
)
