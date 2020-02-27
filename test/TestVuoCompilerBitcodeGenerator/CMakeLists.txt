VuoTest(NAME TestVuoCompilerBitcodeGenerator
	SOURCE TestVuoCompilerBitcodeGenerator.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestVuoCompilerBitcodeGenerator
	PRIVATE
	TestVuoCompiler
)
