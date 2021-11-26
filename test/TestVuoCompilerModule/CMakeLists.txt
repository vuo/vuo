VuoTest(NAME TestVuoCompilerModule
	SOURCE TestVuoCompilerModule.cc
	GMALLOC_ELIGIBLE
)
target_link_libraries(TestVuoCompilerModule
	PRIVATE
	TestVuoCompiler
)
