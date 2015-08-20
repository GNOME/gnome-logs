Feature: Shortcuts

  @quit_via_shortcut
  Scenario: Quit Logs via shortcut
    * Make sure gnome-logs-behave-test is running
    * Hit "<Control>Q"
    Then Logs are not running

  @open_help_via_shortcut
  Scenario: Open help via shortcut
    * Make sure gnome-logs-behave-test is running
    * Hit "<F1>"
    Then Help is shown
