VuoTest(NAME TestVuoCompilerNodeClass
	SOURCE TestVuoCompilerNodeClass.cc
	GMALLOC_ELIGIBLE
)
target_link_libraries(TestVuoCompilerNodeClass
	PRIVATE
	TestVuoCompiler
)
add_dependencies(TestVuoCompilerNodeClass VuoEditorApp)
