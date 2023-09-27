set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -F${PROJECT_BINARY_DIR}/lib")

add_executable(vuo-compile-for-framework
	../vuo-compile/vuo-compile.cc
)
set_target_properties(vuo-compile-for-framework PROPERTIES
	# Don't automatically add `rpath` search paths to this dylib.
	BUILD_WITH_INSTALL_RPATH TRUE
	INSTALL_RPATH @loader_path/../lib
)
target_include_directories(vuo-compile-for-framework
	PRIVATE
		${PROJECT_BINARY_DIR}/compiler
)
target_compile_definitions(vuo-compile-for-framework
	PRIVATE
	USING_VUO_FRAMEWORK
)
target_link_libraries(vuo-compile-for-framework
	VuoFrameworkDylib
)
add_dependencies(vuo-compile-for-framework
	VuoCompilerHash
)
add_custom_command(TARGET vuo-compile-for-framework POST_BUILD COMMAND ${codesignAdHoc} ${PROJECT_BINARY_DIR}/bin/vuo-compile-for-framework)
VuoAddInfoPListSection(vuo-compile-for-framework vuo-compile-Info.plist)
VuoPCH(vuo-compile-for-framework QTestVuoFramework)
