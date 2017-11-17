#!/usr/bin/env python3

import os
import subprocess

schemadir = os.path.join(os.environ['MESON_INSTALL_PREFIX'], 'share', 'glib-2.0', 'schemas')

if not os.environ.get('DESTDIR'):
    print('Compiling gsettings schemas...')
    subprocess.call(['glib-compile-schemas', join_paths(gl_datadir, 'glib-2.0', 'schemas')])
    print('Updating icon cache...')
    subprocess.call(['gtk-update-icon-cache --ignore-theme-index --force', join_paths(gl_datadir, 'icons', 'hicolor')])
