add_executable(vuo-compile
	vuo-compile.cc
)
set_target_properties(vuo-compile PROPERTIES
	BUILD_RPATH ${CONAN_LIB_DIRS_LLVM}
)
target_link_libraries(vuo-compile
	"-framework AppKit"
	VuoCompiler
)
VuoAddInfoPListSection(vuo-compile vuo-compile-Info.plist)
VuoPCH(vuo-compile LLVM)
