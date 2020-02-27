VuoTest(NAME TestVuoCompilerGraphvizParser
	SOURCE TestVuoCompilerGraphvizParser.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestVuoCompilerGraphvizParser
	PRIVATE
	TestVuoCompiler
)
