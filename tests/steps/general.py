from behave import step

from os import system
from pyatspi import STATE_SENSITIVE
from time import sleep
from common_steps import App

@step(u'Open gnome-logs-behave-test')
def run_gnome_logs_test(context):
    system("./gnome-logs-test --force-shutdown 2&> /dev/null")
    context.execute_steps(u'* Start a new gnome-logs-behave-test instance')

@step(u'Click on search')
def click_on_search(context):
    context.app.child('Find').click()
    
@then(u'search is focused and selection toolbar buttons are sensitive')
def search_focused_selection_toolbar_buttons_sensitive(context):
    assert context.app.child('Search').focused
    assert context.app.child('Important').sensitive
    assert context.app.child('All').sensitive
    assert context.app.child('Applications').sensitive
    assert context.app.child('System').sensitive
    assert context.app.child('Security').sensitive
    assert context.app.child('Hardware').sensitive
    context.app.child('Search').typeText("test")
    assert context.app.child('This is a test').sensitive

@step(u'Type search text')
def type_search_text(context):
    context.app.child('Search').typeText("test")

@step(u'assert test')
def assert_test(context):
    assert context.app.child('Search').focused
    assert context.app.child('This is a test').sensitive

@step(u'Go Back')
def go_back(context):
    context.app.child('This is a test').click()

@step(u'Select the log listing')
def select_log_listing(context):
    context.app.child('This is a test').click()

@step(u'Press the back button')
def press_back_button(context):
    context.app.child('Back').click()

@step('return to the main window')
def return_main_window(context):
    context.app.child('Window').sensitive

