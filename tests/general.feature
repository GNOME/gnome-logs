Feature: General

@start_logs
  Scenario: Search
    * Make sure gnome-logs-behave-test is running
    * Click on Search
    Then search is focused and selection toolbar buttons are sensitive

  Scenario: Search with text
    * Make sure gnome-logs-behave-test is running
    * Type search text
    Then assert test

  Scenario: Go Back
    * Make sure gnome-logs-behave-test is running
    * Select the log listing
    * Assert the message in details view
    * Press the back button
    Then return to the main window

  @open_help_via_menu
  Scenario: Open help from menu
    * Make sure gnome-logs-behave-test is running
    * Select "Help" from supermenu
    Then Help is shown

  @quit_via_panel
  Scenario: Quit Logs via super menu
    * Make sure gnome-logs-behave-test is running
    * Select "Quit" from supermenu
    Then Logs are not running

  @open_about_via_menu
  Scenario: Open about from menu
    * Make sure gnome-logs-behave-test is running
    * Select "About" from supermenu
    * Press "Credits"
    * Press "About"
    Then About is shown
