set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -F${PROJECT_BINARY_DIR}/lib")

add_executable(vuo-compile-for-framework
	../vuo-compile/vuo-compile.cc
)
set_target_properties(vuo-compile-for-framework PROPERTIES
	# Don't automatically add `rpath` search paths to this dylib.
	BUILD_WITH_INSTALL_RPATH TRUE
	INSTALL_RPATH @loader_path/../lib
)
target_compile_definitions(vuo-compile-for-framework
	PRIVATE
	USING_VUO_FRAMEWORK
)
target_link_libraries(vuo-compile-for-framework
	VuoFrameworkDylib
)
VuoAddInfoPListSection(vuo-compile-for-framework vuo-compile-Info.plist)
VuoPCH(vuo-compile-for-framework QTestVuoFramework)
