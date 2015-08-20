# -*- coding: UTF-8 -*-

from __future__ import unicode_literals
from behave import step
from dogtail.rawinput import typeText, pressKey, keyCombo
from time import sleep
from subprocess import call, check_output, CalledProcessError, STDOUT

@step('Hit "{keycombo}"')
def hit_keycombo(context, keycombo):
    sleep(0.2)
    if keycombo == "Enter":
        pressKey("Enter")
    else:
        keyCombo('%s'%keycombo)
