VuoTest(NAME TestEventDropping
	SOURCE TestEventDropping.cc

	# This test installs modules to shared folders, which can trip up other tests.
	RUN_SERIAL ON
)
target_link_libraries(TestEventDropping
	PRIVATE
	TestCompositionExecution
)

VuoCompileNodes(
	node-TestEventDropping/vuo.test.delay.c
)
add_dependencies(TestEventDropping TestEventDropping.nodes)
