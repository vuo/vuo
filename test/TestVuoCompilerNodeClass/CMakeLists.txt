VuoTest(NAME TestVuoCompilerNodeClass
	SOURCE TestVuoCompilerNodeClass.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestVuoCompilerNodeClass
	PRIVATE
	TestVuoCompiler
)
add_dependencies(TestVuoCompilerNodeClass VuoEditorApp)
