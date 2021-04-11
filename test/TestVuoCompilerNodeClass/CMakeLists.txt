VuoTest(NAME TestVuoCompilerNodeClass
	SOURCE TestVuoCompilerNodeClass.cc
)
target_link_libraries(TestVuoCompilerNodeClass
	PRIVATE
	TestVuoCompiler
)
add_dependencies(TestVuoCompilerNodeClass VuoEditorApp)
