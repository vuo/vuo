VuoTest(NAME TestEventDropping
	SOURCE TestEventDropping.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestEventDropping
	PRIVATE
	TestCompositionExecution
)

VuoCompileNodes(
	node-TestEventDropping/vuo.test.delay.c
)
add_dependencies(TestEventDropping TestEventDropping.nodes)
