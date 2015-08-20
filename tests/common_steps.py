# -*- coding: UTF-8 -*-
from dogtail.utils import isA11yEnabled, enableA11y, run
if isA11yEnabled() is False:
    enableA11y(True)

from time import time, sleep
from functools import wraps
from os import strerror, errno, system
from signal import signal, alarm, SIGALRM
from subprocess import Popen, PIPE
from behave import step
from gi.repository import GLib, Gio
import os

from dogtail.rawinput import keyCombo, absoluteMotion, pressKey
from dogtail.tree import root
from unittest import TestCase


# Create a dummy unittest class to have nice assertions
class dummy(TestCase):
    def runTest(self):  # pylint: disable=R0201
        assert True

class TimeoutError(Exception):
    """
    Timeout exception class for limit_execution_time_to function
    """
    pass

class App(object):
    """
    This class does all basic events with the app
    """
    def __init__(
        self, appName, shortcut='<Control><Q>', a11yAppName=None,
            forceKill=True, parameters='', recordVideo=False):
        """
        Initialize object App
        appName     command to run the app
        shortcut    default quit shortcut
        a11yAppName app's a11y name is different than binary
        forceKill   is the app supposed to be kill before/after test?
        parameters  has the app any params needed to start? (only for startViaCommand)
        recordVideo start gnome-shell recording while running the app
        """
        self.appCommand = appName
        self.shortcut = shortcut
        self.forceKill = forceKill
        self.parameters = parameters
        self.internCommand = self.appCommand.lower()
        self.a11yAppName = a11yAppName
        self.recordVideo = recordVideo
        self.pid = None

        # a way of overcoming overview autospawn when mouse in 1,1 from start
        pressKey('Esc')
        absoluteMotion(100, 100, 2)

        # attempt to make a recording of the test
        if self.recordVideo:
            keyCombo('<Control><Alt><Shift>R')

    def isRunning(self):
        """
        Is the app running?
        """
        if self.a11yAppName is None:
            self.a11yAppName = self.internCommand
	self.a11yAppName = 'gnome-logs'

        # Trap weird bus errors
        for attempt in xrange(0, 30):
            sleep(1)
            try:
                return self.a11yAppName in [x.name for x in root.applications()]
            except GLib.GError:
                continue
        raise Exception("10 at-spi errors, seems that bus is blocked")

    def kill(self):
        """
        Kill the app via 'killall'
        """
        if self.recordVideo:
            keyCombo('<Control><Alt><Shift>R')

        try:
            self.process.kill()
        except:
            # Fall back to killall
            Popen("killall " + self.appCommand, shell=True).wait()

    def startViaCommand(self):
        """
        Start the app via command
        """
        if self.forceKill and self.isRunning():
            self.kill()
            assert not self.isRunning(), "Application cannot be stopped"

        #command = "%s %s" % (self.appCommand, self.parameters)
        #self.pid = run(command, timeout=5)
        self.process = Popen(self.appCommand.split() + self.parameters.split(),
                             stdout=PIPE, stderr=PIPE, bufsize=0)
        self.pid = self.process.pid

        assert self.isRunning(), "Application failed to start"
        return root.application(self.a11yAppName)

    def closeViaShortcut(self):
        """
        Close the app via shortcut
        """
        if not self.isRunning():
            raise Exception("App is not running")

        keyCombo(self.shortcut)
        assert not self.isRunning(), "Application cannot be stopped"


@step(u'Make sure gnome-logs-behave-test is running')
def ensure_app_running(context):
    context.app = context.app_class.startViaCommand()

def cleanup():
    # Remove cached data and settings
    folders = ['~/.local/share/gnome-logs-test', '~/.cache/gnome-logs/test', '~/.config/gnome-logs-test']
    for folder in folders:
        system("rm -rf %s > /dev/null" % folder)

    # Reset GSettings
    schemas = [x for x in Gio.Settings.list_schemas() if 'gnome-logs-test' in x.lower()]
    for schema in schemas:
        system("gsettings reset-recursively %s" % schema)
