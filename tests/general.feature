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
    * Press the back button
    Then return to the main window
