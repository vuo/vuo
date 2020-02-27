set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -F${PROJECT_BINARY_DIR}/lib")

add_executable(vuo-link
	vuo-link.cc
)
set_target_properties(vuo-link PROPERTIES
	# Don't automatically add `rpath` search paths to this dylib.
	BUILD_WITH_INSTALL_RPATH TRUE
	INSTALL_RPATH @loader_path/../lib
)
target_link_libraries(vuo-link
	VuoFrameworkDylib
)
VuoAddInfoPListSection(vuo-link vuo-link-Info.plist)
VuoPCH(vuo-link QTestVuoFramework)
