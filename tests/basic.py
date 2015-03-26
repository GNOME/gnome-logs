#! /usr/bin/python

# This a simple test, using the dogtail framework:
#
# Click on Important, All, Applications, System, Security, Hardware and check
# that the button state and the log listings update as expected.

from gi.repository import Gio

settings = Gio.Settings.new('org.gnome.desktop.interface')
settings.set_boolean('toolkit-accessibility', True)

import os
from dogtail.tree import *
from dogtail.utils import *
from dogtail.procedural import *

try:
    run('gnome-logs')
    app_name = 'gnome-logs'
    app = root.application(app_name)

    important_button = app.child('Important').parent
    all_button = app.child('All').parent
    applications_button = app.child('Applications').parent
    system_button = app.child('System').parent
    security_button = app.child('Security').parent
    hardware_button = app.child('Hardware').parent

    important_button.click()
    assert (important_button.isSelected)
    assert (not all_button.isSelected)
    assert (not applications_button.isSelected)
    assert (not system_button.isSelected)
    assert (not security_button.isSelected)
    assert (not hardware_button.isSelected)

    all_button.click()
    assert (not important_button.isSelected)
    assert (all_button.isSelected)
    assert (not applications_button.isSelected)
    assert (not system_button.isSelected)
    assert (not security_button.isSelected)
    assert (not hardware_button.isSelected)

    applications_button.click()
    assert (not important_button.isSelected)
    assert (not all_button.isSelected)
    assert (applications_button.isSelected)
    assert (not system_button.isSelected)
    assert (not security_button.isSelected)
    assert (not hardware_button.isSelected)

    system_button.click()
    assert (not important_button.isSelected)
    assert (not all_button.isSelected)
    assert (not applications_button.isSelected)
    assert (system_button.isSelected)
    assert (not security_button.isSelected)
    assert (not hardware_button.isSelected)

    security_button.click()
    assert (not important_button.isSelected)
    assert (not all_button.isSelected)
    assert (not applications_button.isSelected)
    assert (not system_button.isSelected)
    assert (security_button.isSelected)
    assert (not hardware_button.isSelected)

    hardware_button.click()
    assert (not important_button.isSelected)
    assert (not all_button.isSelected)
    assert (not applications_button.isSelected)
    assert (not system_button.isSelected)
    assert (not security_button.isSelected)
    assert (hardware_button.isSelected)
finally:
    os.system('killall gnome-logs')
