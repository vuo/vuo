Feature: Incremental builds
Check that modifying certain source files causes certain build steps to be performed (or not performed).

   Scenario: Running CMake without other changes
      Given a completed build
      When  I run CMake
      And   I build
      Then  it shouldn't do anything

   Scenario: No changes
      Given a completed build
      When  I build
      Then  it shouldn't do anything

   Scenario: Editing a core type header (VuoInteger.h)
      Given a completed build
      When  I edit type/VuoInteger.h
      And   I build

      Then  it should update lib/Vuo.framework/Headers/VuoInteger.h
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Headers/VuoInteger.h

      And   it should update type/list/VuoList_VuoInteger.bc
      And   it should update type/list/VuoList_VuoInteger.o
      And   it should update lib/Vuo.framework/Modules/VuoList_VuoInteger.bc
      And   it should update lib/Vuo.framework/Modules/VuoList_VuoInteger.o
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/VuoList_VuoInteger.bc
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/VuoList_VuoInteger.o

      And   it should update the built-in cache

      When  I build
      Then  it shouldn't do anything

   Scenario: Editing a core type header (VuoShader.h)
      Given a completed build
      When  I edit type/VuoShader.h
      And   I build

      Then  it should update lib/Vuo.framework/Headers/VuoShader.h
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Headers/VuoShader.h

      # VuoSceneRenderer.cc implicitly depends on VuoShader.h (via `#include`).
      And   it should update library/VuoSceneRenderer.bc
      And   it should update library/VuoSceneRenderer.o
      And   it should update lib/Vuo.framework/Modules/VuoSceneRenderer.bc
      And   it should update lib/Vuo.framework/Modules/VuoSceneRenderer.o
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/VuoSceneRenderer.bc
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/VuoSceneRenderer.o

      And   it should update the built-in cache

      When  I build
      Then  it shouldn't do anything

   Scenario: Editing a core type implementation
      Given a completed build
      When  I edit type/VuoInteger.c
      And   I build

      Then  it should update lib/Vuo.framework/Modules/VuoInteger.bc
      And   it should update lib/Vuo.framework/Modules/VuoInteger.o
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/VuoInteger.bc
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/VuoInteger.o

      And   it should update the built-in cache

      When  I build
      Then  it shouldn't do anything

   Scenario: Editing a node in a node set
      Given a completed build
      When  I edit node/vuo.event/vuo.event.fireOnStart.c
      And   I build
      Then  it should update the vuo.event node set
      And   it should update the built-in cache

      When  I build
      Then  it shouldn't do anything

   Scenario: Editing a type in a node set
      Given a completed build
      When  I edit node/vuo.color/VuoDmxColorMap.c
      And   I build
      Then  it should update the vuo.color node set
      And   it should update the built-in cache

      When  I build
      Then  it shouldn't do anything

   Scenario: Editing a description in a node set
      Given a completed build
      When  I edit node/vuo.color/descriptions/vuo.color.average.md
      And   I build
      Then  it should update the vuo.color node set
      But   it shouldn't update the built-in cache

      When  I build
      Then  it shouldn't do anything

   Scenario: Editing an example in a node set
      Given a completed build
      When  I edit node/vuo.motion/examples/CompareEasingCurves.vuo
      And   I build
      Then  it should update the vuo.motion node set
      But   it shouldn't update the built-in cache

      When  I build
      Then  it shouldn't do anything

   Scenario: Editing the list type template implementation
      Given a completed build
      When  I edit type/list/VuoList.cc
      And   I build

      # It should update all list types.  This test uses VuoList_VuoInteger as an example.
      Then  it should update type/list/VuoList_VuoInteger.bc
      And   it should update type/list/VuoList_VuoInteger.o
      And   it should update lib/Vuo.framework/Modules/VuoList_VuoInteger.bc
      And   it should update lib/Vuo.framework/Modules/VuoList_VuoInteger.o
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/VuoList_VuoInteger.bc
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/VuoList_VuoInteger.o

      And   it should update the built-in cache

      When  I build
      Then  it shouldn't do anything

   Scenario: Editing the list type template header
      Given a completed build
      When  I edit type/list/VuoList.h
      And   I build

      # It should update all list types, and all types that use lists.  This tests just a few examples.
      Then  it should update type/VuoShader.bc
      And   it should update type/VuoShader.o
      And   it should update lib/Vuo.framework/Modules/VuoShader.bc
      And   it should update lib/Vuo.framework/Modules/VuoShader.o
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/VuoShader.bc
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/VuoShader.o

      And   it should update type/list/VuoList_VuoInteger.bc
      And   it should update type/list/VuoList_VuoInteger.o
      And   it should update lib/Vuo.framework/Headers/VuoList_VuoInteger.h
      And   it should update lib/Vuo.framework/Modules/VuoList_VuoInteger.bc
      And   it should update lib/Vuo.framework/Modules/VuoList_VuoInteger.o
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Headers/VuoList_VuoInteger.h
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/VuoList_VuoInteger.bc
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Modules/VuoList_VuoInteger.o

      And   it should update the built-in cache

      When  I build
      Then  it shouldn't do anything

   Scenario: Editing the master framework header
      Given a completed build
      When  I edit framework/Vuo.stub.h
      And   I build
      Then  it should update lib/Vuo.framework/Headers/Vuo.h
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Headers/Vuo.h

      When  I build
      Then  it shouldn't do anything

   Scenario: Editing VuoCompositionLoader
      Given a completed build
      When  I edit runtime/VuoCompositionLoader.cc
      And   I build
      Then  it should update bin/VuoCompositionLoader.app/Contents/MacOS/VuoCompositionLoader
      And   it should update lib/Vuo.framework/Helpers/VuoCompositionLoader.app/Contents/MacOS/VuoCompositionLoader
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Helpers/VuoCompositionLoader.app/Contents/MacOS/VuoCompositionLoader

      When  I build
      Then  it shouldn't do anything

   Scenario: Editing an input editor
      Given a completed build
      When  I edit type/inputEditor/VuoInputEditorReal/VuoInputEditorReal.cc
      And   I build
      Then  it should update lib/libVuoInputEditorReal.dylib
      And   it should update bin/Vuo.app/Contents/Resources/InputEditors/libVuoInputEditorReal.dylib
      But   it shouldn't update the built-in cache

      When  I build
      Then  it shouldn't do anything

   Scenario: Editing a type used by an input editor
      Given a completed build
      When  I edit node/vuo.scene/VuoDispersion.c
      And   I build
      Then  it should update the vuo.scene node set
      And   it should update lib/Vuo.framework/Vuo
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Vuo
      And   it should update the built-in cache

      # These binaries dynamically link to Vuo.framework,
      # so they don't need to be re-linked.
      But   it shouldn't update lib/libVuoInputEditorDispersion.dylib
      And   it shouldn't update bin/Vuo.app/Contents/Resources/InputEditors/libVuoInputEditorDispersion.dylib
      And   it shouldn't update bin/Vuo.app/Contents/MacOS/Vuo

      When  I build
      Then  it shouldn't do anything

   Scenario: Editing a module-compiler
      Given a completed build
      When  I edit compiler/isf/VuoIsfModuleCompiler.cc
      And   I build
      Then  it should update lib/libVuoIsfModuleCompiler.dylib
      And   it should update lib/Vuo.framework/Helpers/ModuleCompiler/libVuoIsfModuleCompiler.dylib
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Helpers/ModuleCompiler/libVuoIsfModuleCompiler.dylib
      But   it shouldn't update the built-in cache

      When  I build
      Then  it shouldn't do anything

   Scenario: Editing a stub
      Given a completed build
      When  I edit renderer/stub/VuoFxFilter.mm
      And   I build
      Then  it should update bin/VuoFxFilter
      And   it should update lib/Vuo.framework/Resources/VuoFxFilter
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Resources/VuoFxFilter
      But   it shouldn't update the built-in cache

      When  I build
      Then  it shouldn't do anything

   Scenario: Editing a .ui file
      Given a completed build
      When  I edit editor/VuoEditorWindow.ui
      And   I build
      Then  it should update editor/CMakeFiles/VuoEditor.dir/VuoEditorWindow.cc.o
      And   it should update bin/Vuo.app/Contents/MacOS/Vuo
      But   it shouldn't update the built-in cache
      # Fails when building with Ninja due to CMake bug: https://gitlab.kitware.com/cmake/cmake/issues/16776

      When  I build
      Then  it shouldn't do anything

   Scenario: Editing a shader template
      Given a completed build
      When  I edit framework/templates/GLSLImageTransition.fs
      And   I build
      Then  it should update lib/Vuo.framework/Resources/GLSLImageTransition.fs
      And   it should update bin/Vuo.app/Contents/Frameworks/Vuo.framework/Resources/GLSLImageTransition.fs
      But   it shouldn't update the built-in cache

      When  I build
      Then  it shouldn't do anything
