VuoTest(NAME TestVuoCompilerNode
	SOURCE TestVuoCompilerNode.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestVuoCompilerNode
	PRIVATE
	TestVuoCompiler
)
