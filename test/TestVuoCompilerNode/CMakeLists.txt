VuoTest(NAME TestVuoCompilerNode
	SOURCE TestVuoCompilerNode.cc
	GMALLOC_ELIGIBLE
)
target_link_libraries(TestVuoCompilerNode
	PRIVATE
	TestVuoCompiler
)
