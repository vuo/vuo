add_library(VuoCompiler STATIC
	VuoCompiler.cc
	VuoCompilerBitcodeGenerator.cc
	VuoCompilerBitcodeParser.cc
	VuoCompilerCable.cc
	VuoCompilerChain.cc
	VuoCompilerCodeGenUtilities.cc
	VuoCompilerComment.cc
	VuoCompilerComposition.cc
	VuoCompilerCompositionDiff.cc
	VuoCompilerConstantStringCache.cc
	VuoCompilerData.cc
	VuoCompilerDataClass.cc
	VuoCompilerDriver.cc
	VuoCompilerEventPort.cc
	VuoCompilerEventPortClass.cc
	VuoCompilerException.cc
	VuoCompilerGenericType.cc
	VuoCompilerGraph.cc
	VuoCompilerGraphvizParser.cc
	VuoCompilerInputData.cc
	VuoCompilerInputDataClass.cc
	VuoCompilerInputEventPort.cc
	VuoCompilerInputEventPortClass.cc
	VuoCompilerInstanceData.cc
	VuoCompilerInstanceDataClass.cc
	VuoCompilerIssue.cc
	VuoCompilerMakeListNodeClass.cc
	VuoCompilerModule.cc
	VuoCompilerNode.cc
	VuoCompilerNodeArgument.cc
	VuoCompilerNodeArgumentClass.cc
	VuoCompilerNodeClass.cc
	VuoCompilerOutputData.cc
	VuoCompilerOutputDataClass.cc
	VuoCompilerOutputEventPort.cc
	VuoCompilerOutputEventPortClass.cc
	VuoCompilerPort.cc
	VuoCompilerPortClass.cc
	VuoCompilerPublishedInputNodeClass.cc
	VuoCompilerPublishedNodeClass.cc
	VuoCompilerPublishedOutputNodeClass.cc
	VuoCompilerPublishedPort.cc
	VuoCompilerPublishedPortClass.cc
	VuoCompilerSpecializedNodeClass.cc
	VuoCompilerTargetSet.cc
	VuoCompilerTriggerDescription.cc
	VuoCompilerTriggerPort.cc
	VuoCompilerTriggerPortClass.cc
	VuoCompilerType.cc
	VuoDirectedAcyclicGraph.cc
	VuoModuleCompiler.cc
	VuoModuleCompilationQueue.cc
	VuoRunnerCocoa+Conversion.mm
	VuoRunnerCocoa.mm
)
target_compile_definitions(VuoCompiler
	PUBLIC
	LLVM
	ZMQ
)
target_include_directories(VuoCompiler
	PUBLIC
		${PROJECT_BINARY_DIR}/type/list
		../library
		../type
		.
)
target_link_libraries(VuoCompiler PUBLIC
	"-framework QuartzCore"
	CONAN_PKG::graphviz

	# Don't use the default LLVM target here, since it links libraries we don't need.
#	CONAN_PKG::llvm
	# Instead, use our trimmed version (from trunk/CMakeLists.txt).
	-L${CONAN_LIB_DIRS_LLVM} ${CONAN_LIBS_LLVM}
	c++
	c++abi

	VuoBase
	VuoHeap
)
if (VuoPro)
	include(pro/pro.cmake)
endif()
VuoPCH(VuoCompiler LLVM)

add_subdirectory(isf)
add_subdirectory(vuo-compile)
add_subdirectory(vuo-compile-for-framework)
add_subdirectory(vuo-debug)
add_subdirectory(vuo-link)
