VuoTest(NAME TestVuoCompilerGraphvizParser
	SOURCE TestVuoCompilerGraphvizParser.cc
)
target_link_libraries(TestVuoCompilerGraphvizParser
	PRIVATE
	TestVuoCompiler
)
