VuoTest(NAME TestVuoCompilerType
	SOURCE TestVuoCompilerType.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestVuoCompilerType
	PRIVATE
	TestVuoCompiler
)
