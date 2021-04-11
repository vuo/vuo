VuoInputEditor(NAME VuoInputEditorFont SOURCE VuoInputEditorFont.mm)
target_link_libraries(VuoInputEditorFont PRIVATE
	"-framework AppKit"
)

# Workaround for apparent CMake bug where it uses the wrong path to the precompiled header when the only source file is OBJCXX.
set_property(SOURCE VuoInputEditorFont_autogen/mocs_compilation.cpp PROPERTY SKIP_PRECOMPILE_HEADERS ON)
set_property(SOURCE VuoInputEditorFont_autogen/mocs_compilation.cpp APPEND PROPERTY COMPILE_FLAGS "-include ${PROJECT_SOURCE_DIR}/vuo.pch")
set_property(SOURCE VuoInputEditorFont_autogen/mocs_compilation.cpp APPEND PROPERTY COMPILE_DEFINITIONS VUO_PCH_QT)
