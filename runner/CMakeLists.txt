add_library(VuoRunner STATIC
	VuoRunner.cc
	VuoRunner.hh
)
target_include_directories(VuoRunner
	PRIVATE
		../base
		../type
		${PROJECT_BINARY_DIR}/type
)
target_link_libraries(VuoRunner
	PRIVATE
		VuoHeap
)
if (VUO_RUNNER_TRACE)
	target_compile_definitions(VuoRunner PRIVATE VUO_RUNNER_TRACE)
endif()
add_dependencies(VuoRunner vuo.core.types.compound.headers)
VuoPCH(VuoRunner ModuleOnly)


add_library(VuoRunnerCocoa STATIC
	VuoRunnerCocoa+Conversion.hh
	VuoRunnerCocoa+Conversion.mm
	VuoRunnerCocoa.h
	VuoRunnerCocoa.mm
)
target_include_directories(VuoRunnerCocoa PRIVATE
	../base
	../compiler
	../type
	${PROJECT_BINARY_DIR}/type
)
if (VuoPro)
	include(pro/pro.cmake)
endif()
add_dependencies(VuoRunnerCocoa vuo.core.types.compound.headers)
VuoPCH(VuoRunnerCocoa LLVMModule)
