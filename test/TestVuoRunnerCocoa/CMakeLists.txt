VuoTest(NAME TestVuoRunnerCocoa
	SOURCE TestVuoRunnerCocoa.mm
)
target_link_libraries(TestVuoRunnerCocoa
	PUBLIC
	"-framework AppKit"
	"-framework CoreVideo"
	"-framework OpenGL"
	"-framework QuartzCore"
)

# Workaround for apparent CMake bug where it uses the wrong path to the precompiled header when the only source file is OBJCXX.
set_property(SOURCE TestVuoRunnerCocoa_autogen/mocs_compilation.cpp PROPERTY SKIP_PRECOMPILE_HEADERS ON)
set_property(SOURCE TestVuoRunnerCocoa_autogen/mocs_compilation.cpp APPEND PROPERTY COMPILE_FLAGS "-include ${PROJECT_SOURCE_DIR}/vuo.pch")
set_property(SOURCE TestVuoRunnerCocoa_autogen/mocs_compilation.cpp APPEND PROPERTY COMPILE_DEFINITIONS VUO_PCH_QTEST VUO_PCH_FRAMEWORK)
