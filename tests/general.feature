Feature: General

Background:
  * Make sure that gnome-logs-test is running

@start_logs
  Scenario: Search
    * Click on Search
    Then all selection toolbar buttons are sensitive
