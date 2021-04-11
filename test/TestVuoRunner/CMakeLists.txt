VuoTest(NAME TestVuoRunner
	SOURCE TestVuoRunner.cc
)
target_link_libraries(TestVuoRunner
	PUBLIC
	"-framework OpenGL"
	CONAN_PKG::zeromq
)


string(REPLACE "-Xlinker -dead_strip" "" CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
function (TestRunnerSource target)
	add_executable(${target} EXCLUDE_FROM_ALL ${target}.c)
	target_include_directories(${target}
		PRIVATE
			${PROJECT_SOURCE_DIR}/base
	)
	target_link_libraries(${target}
		CONAN_PKG::jsonc
		CONAN_PKG::zeromq
		VuoApp
		VuoHeap
		vuo.core.runtime.objects
	)
	set_target_properties(${target} PROPERTIES LINKER_LANGUAGE CXX)
	add_dependencies(TestVuoRunner ${target})
endfunction()

TestRunnerSource(compositionForListening)
TestRunnerSource(compositionForControlling)
TestRunnerSource(compositionForLosingContact)


VuoCompileCompositions(TestVuoRunnerCompositions
	PublishedPorts.vuo
)
add_dependencies(TestVuoRunner
	TestVuoRunnerCompositions
	VuoEditorApp
)
