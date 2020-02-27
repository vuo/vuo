Feature: Compiler developer mode
Check that compiler developer mode properly modifies build dependencies.

   Scenario: Enabling compiler developer mode
      Given a completed build
      When  I run CMake with arguments "-DVUO_COMPILER_DEVELOPER=ON"
      And   I build
      Then  it shouldn't do anything

   Scenario: Editing a compiler source file when in compiler developer mode
      Given a completed build
      When  I edit compiler/VuoCompiler.cc
      And   I build

      Then  it should update lib/libVuoCompiler.a
      And   it should update bin/vuo-compile
      And   it should update lib/Vuo.framework/Vuo
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Vuo
      And   it should update bin/Vuo.app/Contents/MacOS/Vuo

      But   it shouldn't update the built-in cache

   Scenario: Disabling compiler developer mode
      Given a completed build
      When  I run CMake with arguments "-DVUO_COMPILER_DEVELOPER=OFF"
      And   I build

      # Because VuoCompiler.cc was edited above, it should now update all types and nodes.  This tests just a few examples.
      Then  it should update type/VuoInteger.bc
      And   it should update type/VuoInteger.o
      And   it should update the vuo.color node set
      And   it should update the built-in cache
