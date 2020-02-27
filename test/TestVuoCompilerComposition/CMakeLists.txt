VuoTest(NAME TestVuoCompilerComposition
	SOURCE TestVuoCompilerComposition.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestVuoCompilerComposition
	PRIVATE
	TestVuoCompiler
)
