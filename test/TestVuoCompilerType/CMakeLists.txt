VuoTest(NAME TestVuoCompilerType
	SOURCE TestVuoCompilerType.cc
)
target_link_libraries(TestVuoCompilerType
	PRIVATE
	TestVuoCompiler
)
