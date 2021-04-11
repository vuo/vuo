VuoTest(NAME TestVuoCompilerComposition
	SOURCE TestVuoCompilerComposition.cc
)
target_link_libraries(TestVuoCompilerComposition
	PRIVATE
	TestVuoCompiler
)
