from behave import step

from os import system
from pyatspi import STATE_SENSITIVE
from time import sleep
from common_steps import App

@step(u'Run gnome-logs-test')
def run_gnome_logs_test(context):
    system("./gnome-logs-test --force-shutdown 2&> /dev/null")
    context.execute_steps(u'* Start a new Logs instance')

@step(u'Click on search')
def click_on_search(context):
    context.app.child('Search').parent.click()
    
@then(u'all selection toolbar buttons are sensitive')
def all_selection_toolbar_buttons_sensitive(context):
    sleep(0.5)
    assert context.app.child(translate('Important')).sensitive
    assert context.app.child(translate('All')).sensitive
    assert context.app.child(translate('Applications')).sensitive
    assert context.app.child(translate('System')).sensitive
    assert context.app.child(translate('Security')).sensitive
    assert context.app.child(translate('Hardware')).sensitive
    sleep(0.5) 
