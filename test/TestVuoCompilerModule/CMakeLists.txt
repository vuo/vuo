VuoTest(NAME TestVuoCompilerModule
	SOURCE TestVuoCompilerModule.cc
)
target_link_libraries(TestVuoCompilerModule
	PRIVATE
	TestVuoCompiler
)
