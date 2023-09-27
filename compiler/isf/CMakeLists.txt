add_library(VuoIsfModuleCompiler SHARED
	VuoIsfModuleCompiler.cc
	VuoIsfModuleCompiler.hh
)
set_target_properties(VuoIsfModuleCompiler PROPERTIES
	# Don't automatically add `rpath` search paths to this dylib.
	BUILD_WITH_INSTALL_RPATH TRUE
)
target_include_directories(VuoIsfModuleCompiler
	PRIVATE
		${PROJECT_BINARY_DIR}/lib/Vuo.framework/Versions/${VuoFrameworkVersion}/Headers
)
target_link_libraries(VuoIsfModuleCompiler
	PRIVATE
	VuoFrameworkDylib
)


add_dependencies(VuoModuleCompilers
	VuoIsfModuleCompiler
)
set_property(
	TARGET VuoModuleCompilers
	APPEND PROPERTY VuoGeneratedFiles "${PROJECT_BINARY_DIR}/lib/libVuoIsfModuleCompiler.dylib"
)
