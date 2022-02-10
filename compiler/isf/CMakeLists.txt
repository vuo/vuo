add_library(VuoIsfModuleCompiler SHARED
	VuoIsfModuleCompiler.cc
	VuoIsfModuleCompiler.hh
)
set_target_properties(VuoIsfModuleCompiler PROPERTIES
	AUTOMOC ON

	# Don't automatically add `rpath` search paths to this dylib.
	BUILD_WITH_INSTALL_RPATH TRUE
)
target_include_directories(VuoIsfModuleCompiler
	PRIVATE
		${PROJECT_BINARY_DIR}/lib/Vuo.framework/Headers
)
target_link_libraries(VuoIsfModuleCompiler
	PRIVATE
	VuoFrameworkDylib
)


add_custom_target(VuoModuleCompilers
	DEPENDS
		VuoIsfModuleCompiler
)
set_target_properties(VuoModuleCompilers PROPERTIES VuoGeneratedFiles ${PROJECT_BINARY_DIR}/lib/libVuoIsfModuleCompiler.dylib)
