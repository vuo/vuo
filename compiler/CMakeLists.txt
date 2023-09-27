add_library(VuoCompiler STATIC
	VuoClangIssues.cc
	VuoClangIssues.hh
	VuoCompiler.cc
	VuoCompiler.hh
	VuoCompilerBitcodeGenerator.cc
	VuoCompilerBitcodeGenerator.hh
	VuoCompilerBitcodeParser.cc
	VuoCompilerBitcodeParser.hh
	VuoCompilerCable.cc
	VuoCompilerCable.hh
	VuoCompilerChain.cc
	VuoCompilerChain.hh
	VuoCompilerCodeGenUtilities.cc
	VuoCompilerCodeGenUtilities.hh
	VuoCompilerComment.cc
	VuoCompilerComment.hh
	VuoCompilerCompatibility.cc
	VuoCompilerCompatibility.hh
	VuoCompilerComposition.cc
	VuoCompilerComposition.hh
	VuoCompilerCompositionDiff.cc
	VuoCompilerCompositionDiff.hh
	VuoCompilerCompoundType.cc
	VuoCompilerCompoundType.hh
	VuoCompilerConstantsCache.cc
	VuoCompilerConstantsCache.hh
	VuoCompilerData.cc
	VuoCompilerData.hh
	VuoCompilerDataClass.cc
	VuoCompilerDataClass.hh
	VuoCompilerDelegate.cc
	VuoCompilerDelegate.hh
	VuoCompilerDiagnosticConsumer.cc
	VuoCompilerDiagnosticConsumer.hh
	VuoCompilerDriver.cc
	VuoCompilerDriver.hh
	VuoCompilerEnvironment.cc
	VuoCompilerEnvironment.hh
	VuoCompilerEventPort.cc
	VuoCompilerEventPort.hh
	VuoCompilerEventPortClass.cc
	VuoCompilerEventPortClass.hh
	VuoCompilerException.cc
	VuoCompilerException.hh
	VuoCompilerGenericType.cc
	VuoCompilerGenericType.hh
	VuoCompilerGraph.cc
	VuoCompilerGraph.hh
	VuoCompilerGraphvizParser.cc
	VuoCompilerGraphvizParser.hh
	VuoCompilerGroup.cc
	VuoCompilerGroup.hh
	VuoCompilerInputData.cc
	VuoCompilerInputData.hh
	VuoCompilerInputDataClass.cc
	VuoCompilerInputDataClass.hh
	VuoCompilerInputEventPort.cc
	VuoCompilerInputEventPort.hh
	VuoCompilerInputEventPortClass.cc
	VuoCompilerInputEventPortClass.hh
	VuoCompilerInstanceData.cc
	VuoCompilerInstanceData.hh
	VuoCompilerInstanceDataClass.cc
	VuoCompilerInstanceDataClass.hh
	VuoCompilerIssue.cc
	VuoCompilerIssue.hh
	VuoCompilerMakeListNodeClass.cc
	VuoCompilerMakeListNodeClass.hh
	VuoCompilerModule.cc
	VuoCompilerModule.hh
	VuoCompilerNode.cc
	VuoCompilerNode.hh
	VuoCompilerNodeArgument.cc
	VuoCompilerNodeArgument.hh
	VuoCompilerNodeArgumentClass.cc
	VuoCompilerNodeArgumentClass.hh
	VuoCompilerNodeClass.cc
	VuoCompilerNodeClass.hh
	VuoCompilerOutputData.cc
	VuoCompilerOutputData.hh
	VuoCompilerOutputDataClass.cc
	VuoCompilerOutputDataClass.hh
	VuoCompilerOutputEventPort.cc
	VuoCompilerOutputEventPort.hh
	VuoCompilerOutputEventPortClass.cc
	VuoCompilerOutputEventPortClass.hh
	VuoCompilerPort.cc
	VuoCompilerPort.hh
	VuoCompilerPortClass.cc
	VuoCompilerPortClass.hh
	VuoCompilerPublishedInputNodeClass.cc
	VuoCompilerPublishedInputNodeClass.hh
	VuoCompilerPublishedNodeClass.cc
	VuoCompilerPublishedNodeClass.hh
	VuoCompilerPublishedOutputNodeClass.cc
	VuoCompilerPublishedOutputNodeClass.hh
	VuoCompilerPublishedPort.cc
	VuoCompilerPublishedPort.hh
	VuoCompilerPublishedPortClass.cc
	VuoCompilerPublishedPortClass.hh
	VuoCompilerSpecializedNodeClass.cc
	VuoCompilerSpecializedNodeClass.hh
	VuoCompilerTriggerDescription.cc
	VuoCompilerTriggerDescription.hh
	VuoCompilerTriggerPort.cc
	VuoCompilerTriggerPort.hh
	VuoCompilerTriggerPortClass.cc
	VuoCompilerTriggerPortClass.hh
	VuoCompilerType.cc
	VuoCompilerType.hh
	VuoDependencyGraphVertex.cc
	VuoDependencyGraphVertex.hh
	VuoDirectedAcyclicGraph.cc
	VuoDirectedAcyclicGraph.hh
	VuoLinkerInputs.cc
	VuoLinkerInputs.hh
	VuoMakeDependencies.cc
	VuoMakeDependencies.hh
	VuoModuleCache.cc
	VuoModuleCache.hh
	VuoModuleCacheManifest.cc
	VuoModuleCacheManifest.hh
	VuoModuleCacheRevision.cc
	VuoModuleCacheRevision.hh
	VuoModuleCompilationQueue.cc
	VuoModuleCompilationQueue.hh
	VuoModuleCompiler.cc
	VuoModuleCompiler.hh
	VuoModuleCompilerResults.cc
	VuoModuleCompilerResults.hh
	VuoModuleCompilerSettings.cc
	VuoModuleCompilerSettings.hh
	VuoModuleInfo.cc
	VuoModuleInfo.hh
	VuoModuleInfoIterator.cc
	VuoModuleInfoIterator.hh
)
target_compile_definitions(VuoCompiler
	PUBLIC
	LLVM
)
target_include_directories(VuoCompiler
	PUBLIC
		../library
		../runner
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

VuoDefineArchitectureMacros(VuoCompiler)

add_custom_target(VuoModuleCompilers)
add_subdirectory(c)
add_subdirectory(isf)

add_subdirectory(vuo-link)

file(GLOB compilerSourcesAndDependencies
	${PROJECT_SOURCE_DIR}/base/*.hh
	${PROJECT_SOURCE_DIR}/base/*.cc
	${PROJECT_SOURCE_DIR}/base/*.mm
	${PROJECT_SOURCE_DIR}/compiler/*.hh
	${PROJECT_SOURCE_DIR}/compiler/*.cc
	${PROJECT_SOURCE_DIR}/compiler/*.mm
)
add_custom_command(
	DEPENDS ${compilerSourcesAndDependencies}
	COMMENT "Calculating hash of compiler source code"
	COMMAND
		cat ${compilerSourcesAndDependencies}
		| /usr/bin/shasum --algorithm 256
		| perl -pe "s/^\\(.*\\)  -$$/const char *VuoCompilerHash = '\"'\\\\1'\"'\;/"
		> VuoCompilerHash.h
	OUTPUT VuoCompilerHash.h
)
add_custom_target(VuoCompilerHash DEPENDS VuoCompilerHash.h)
add_subdirectory(vuo-compile)
add_subdirectory(vuo-compile-for-framework)
