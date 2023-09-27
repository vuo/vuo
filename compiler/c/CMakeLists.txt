add_library(VuoCModuleCompiler SHARED
	VuoCModuleCompiler.cc
	VuoCModuleCompiler.hh
	VuoEmitLLVMOnlyAction.cc
	VuoEmitLLVMOnlyAction.hh
	VuoReplaceGenericTypesWithSpecializedAction.cc
	VuoReplaceGenericTypesWithSpecializedAction.hh
	VuoPreprocessAction.cc
	VuoPreprocessAction.hh
	VuoPreprocessorCallbacks.cc
	VuoPreprocessorCallbacks.hh
)
set_target_properties(VuoCModuleCompiler PROPERTIES
	# Don't automatically add `rpath` search paths to this dylib.
	BUILD_WITH_INSTALL_RPATH TRUE
)
target_include_directories(VuoCModuleCompiler
	PRIVATE
	    ..
		../../base
		../../library
)
target_link_options(VuoCModuleCompiler
	PRIVATE
	    LINKER:-undefined,dynamic_lookup
)
add_dependencies(VuoCModuleCompiler
	VuoCompilerHash
)
add_custom_command(TARGET VuoCModuleCompiler POST_BUILD COMMAND ${codesignAdHoc} ${PROJECT_BINARY_DIR}/lib/libVuoCModuleCompiler.dylib)
VuoPCH(VuoCModuleCompiler LLVM)


add_dependencies(VuoModuleCompilers
	VuoCModuleCompiler
)
set_property(
	TARGET VuoModuleCompilers
	APPEND PROPERTY VuoGeneratedFiles "${PROJECT_BINARY_DIR}/lib/libVuoCModuleCompiler.dylib"
)
