VuoTest(NAME TestVuoIsfModuleCompiler
	SOURCE TestVuoIsfModuleCompiler.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestVuoIsfModuleCompiler
	PRIVATE
	TestCompositionExecution
)
