VuoTest(NAME TestVuoCompilerComposition
	SOURCE TestVuoCompilerComposition.cc
	GMALLOC_ELIGIBLE
)
target_link_libraries(TestVuoCompilerComposition
	PRIVATE
	TestVuoCompiler
)
