Feature: Parallel build
Check that build-order dependencies are correct enough to support highly-parallel building.

   Scenario: 32
      Given I clean the build
      Then  I run CMake
      And   I build with arguments "-j32"

      When  I build
      Then  it shouldn't do anything
