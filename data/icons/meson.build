apps_icons = [
  ['scalable', 'org.gnome.Logs.svg'],
  ['symbolic', 'org.gnome.Logs-symbolic.svg']
]

foreach icon: apps_icons
  install_data(
    join_paths(icon[0], icon[1]),
    install_dir: join_paths(gl_datadir, 'icons', 'hicolor', icon[0], 'apps')
  )
endforeach