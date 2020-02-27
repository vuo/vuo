Feature: Parallel build
Check that build-order dependencies are correct enough to support highly-parallel building.

   Scenario: 48
      Given I clean the build
      Then  I run CMake
      And   I build with arguments "-j48"

      When  I build
      Then  it shouldn't do anything
