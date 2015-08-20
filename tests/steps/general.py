from behave import step

from os import system
from pyatspi import STATE_SENSITIVE
from time import sleep
from common_steps import App
from dogtail.tree import root
from dogtail.rawinput import typeText, pressKey, keyCombo

def get_showing_node_name(name, parent, timeout=30, step=0.25):
    wait = 0
    while len(parent.findChildren(lambda x: x.name == name and x.showing and x.sensitive)) == 0:
        sleep(step)
        wait += step
        if wait == timeout:
            raise Exception("Timeout: Node %s wasn't found showing" %name)

    return parent.findChildren(lambda x: x.name == name and x.showing and x.sensitive)[0]

@step('About is shown')
def about_shown(context):
    assert context.app.child('About Logs') != None, "About window cannot be focused"

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

@step('Help is shown')
def help_shown(context):
    sleep(1)
    yelp = root.application('yelp')
    assert yelp.child('Logs') != None, "Yelp wasn't opened"
    system("killall yelp")

@step('Assert the message in details view')
def assert_message_details_view(context):
    assert context.app.child('This is a test').sensitive

@step('Select "{action}" from supermenu')
def select_menu_action(context, action):
    keyCombo("<Super_L><F10>")
    if action == 'Help':
	pressKey('Down')
    if action == 'About':
	pressKey('Down')
        pressKey('Down')
    if action == 'Quit':
        pressKey('Down')
        pressKey('Down')
	pressKey('Down')
    pressKey('Enter')

@step('Logs are not running')
def logs_not_running(context):
    assert context.app_class.isRunning() != True, "Logs window still visible"

@step('Press "{button}"')
def press_button(context, button):
    get_showing_node_name(button, context.app).click()
    sleep(0.5)
