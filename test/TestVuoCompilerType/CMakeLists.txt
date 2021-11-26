VuoTest(NAME TestVuoCompilerType
	SOURCE TestVuoCompilerType.cc
	GMALLOC_ELIGIBLE
)
target_link_libraries(TestVuoCompilerType
	PRIVATE
	TestVuoCompiler
)
