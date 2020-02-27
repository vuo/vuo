VuoTest(NAME TestNodeExecutionOrder
	SOURCE TestNodeExecutionOrder.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestNodeExecutionOrder
	PRIVATE
	TestCompositionExecution
)

VuoCompileNodes(
	node-TestNodeExecutionOrder/vuo.test.conductor.c
	node-TestNodeExecutionOrder/vuo.test.firePeriodically.c
	node-TestNodeExecutionOrder/vuo.test.semiconductor.c
)
add_dependencies(TestNodeExecutionOrder TestNodeExecutionOrder.nodes)
