Feature: Node developer mode
Check that node developer mode properly modifies build dependencies.

   Scenario: Enabling node developer mode
      Given a completed build
      When  I run CMake with arguments "-DVUO_NODE_DEVELOPER=ON"
      And   I build

      Then  it shouldn't do anything
      And   it shouldn't create lib/Vuo.framework/Modules/Builtin
      And   it shouldn't create bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/Builtin

   Scenario: Editing a node source file when in node developer mode
      Given a completed build
      When  I edit node/vuo.audio/vuo.audio.bitCrush.c
      And   I build

      Then  it should update node/vuo.audio/vuo.audio.bitCrush.vuonode
      And   it should update node/vuo.audio/vuo.audio.vuonode
      And   it should update lib/Vuo.framework/Modules/vuo.audio.vuonode
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/vuo.audio.vuonode

      And   it shouldn't create lib/Vuo.framework/Modules/Builtin
      And   it shouldn't create bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/Builtin

   Scenario: Disabling node developer mode
      Given a completed build
      When  I run CMake with arguments "-DVUO_NODE_DEVELOPER=OFF"
      And   I build

      # Because vuo.audio.bitCrush.c was edited above, it should now update the cache.
      Then  it should update the built-in cache
