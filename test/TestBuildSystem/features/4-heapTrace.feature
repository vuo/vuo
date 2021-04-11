Feature: Heap trace mode
Check that heap trace mode updates the necessary modules.

  Scenario: Enabling heap trace mode
     Given a completed build
     When  I run CMake with arguments "-DVUO_HEAP_TRACE=ON"
     And   I build

     Then  it should update library/CMakeFiles/VuoHeap.dir/VuoHeap.cc.o
     And   it should update lib/libVuoHeap.dylib
     And   it should update lib/Vuo.framework/Versions/Current/Modules/libVuoHeap.dylib
     And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Versions/Current/Modules/libVuoHeap.dylib

     But   it shouldn't update the built-in cache

  Scenario: Disabling heap trace mode
     Given a completed build
     When  I run CMake with arguments "-DVUO_HEAP_TRACE=OFF"
     And   I build

     Then  it should update library/CMakeFiles/VuoHeap.dir/VuoHeap.cc.o
     And   it should update lib/libVuoHeap.dylib
     And   it should update lib/Vuo.framework/Versions/Current/Modules/libVuoHeap.dylib
     And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Versions/Current/Modules/libVuoHeap.dylib

     But   it shouldn't update the built-in cache

   Scenario: Enabling heap trace mode while compiler developer mode is on
      Given a completed build
      When  I run CMake with arguments "-DVUO_HEAP_TRACE=ON -DVUO_COMPILER_DEVELOPER=ON"
      And   I build

      Then  it should update library/CMakeFiles/VuoHeap.dir/VuoHeap.cc.o
      And   it should update lib/libVuoHeap.dylib
      And   it should update lib/Vuo.framework/Versions/Current/Modules/libVuoHeap.dylib
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Versions/Current/Modules/libVuoHeap.dylib

      But   it shouldn't update the built-in cache

   Scenario: Disabling heap trace mode while compiler developer mode is on
      Given a completed build
      When  I run CMake with arguments "-DVUO_HEAP_TRACE=OFF -DVUO_COMPILER_DEVELOPER=ON"
      And   I build

      Then  it should update library/CMakeFiles/VuoHeap.dir/VuoHeap.cc.o
      And   it should update lib/libVuoHeap.dylib
      And   it should update lib/Vuo.framework/Versions/Current/Modules/libVuoHeap.dylib
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Versions/Current/Modules/libVuoHeap.dylib

      But   it shouldn't update the built-in cache

      Then   I run CMake with arguments "-DVUO_COMPILER_DEVELOPER=OFF"
