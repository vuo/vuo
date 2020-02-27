set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -F${PROJECT_BINARY_DIR}/lib")

add_executable(vuo-debug
	vuo-debug.cc
)
set_target_properties(vuo-debug PROPERTIES
	# Don't automatically add `rpath` search paths to this dylib.
	BUILD_WITH_INSTALL_RPATH TRUE
	INSTALL_RPATH @loader_path/../lib
)
target_link_libraries(vuo-debug
	VuoFrameworkDylib
)
VuoAddInfoPListSection(vuo-debug vuo-debug-Info.plist)
VuoPCH(vuo-debug QTestVuoFramework)
