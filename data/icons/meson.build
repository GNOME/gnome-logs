apps_icons = [
  ['16x16', 'gnome-logs.png'],
  ['22x22', 'gnome-logs.png'],
  ['24x24', 'gnome-logs.png'],
  ['32x32', 'gnome-logs.png'],
  ['48x48', 'gnome-logs.png'],
  ['256x256', 'gnome-logs.png'],
  ['512x512', 'gnome-logs.png'],
  ['symbolic', 'gnome-logs-symbolic.svg']
]

foreach icon: apps_icons
  install_data(
    join_paths(icon[0], icon[1]),
    install_dir: join_paths(gl_datadir, 'icons', 'hicolor', icon[0], 'apps')
  )
endforeach
