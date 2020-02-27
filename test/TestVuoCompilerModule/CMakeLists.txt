VuoTest(NAME TestVuoCompilerModule
	SOURCE TestVuoCompilerModule.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestVuoCompilerModule
	PRIVATE
	TestVuoCompiler
)
