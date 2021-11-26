VuoTest(NAME TestVuoCompilerGraphvizParser
	SOURCE TestVuoCompilerGraphvizParser.cc
	GMALLOC_ELIGIBLE
)
target_link_libraries(TestVuoCompilerGraphvizParser
	PRIVATE
	TestVuoCompiler
)
