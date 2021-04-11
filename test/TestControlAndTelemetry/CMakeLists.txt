VuoTest(NAME TestControlAndTelemetry
	SOURCE TestControlAndTelemetry.cc
)
target_link_libraries(TestControlAndTelemetry
	PRIVATE
	TestCompositionExecution
)

VuoCompileNodes(
	node-TestControlAndTelemetry/vuo.test.delay.c
	node-TestControlAndTelemetry/vuo.test.exit.c
	node-TestControlAndTelemetry/vuo.test.finiCallbackCreatesFile.c
	node-TestControlAndTelemetry/vuo.test.firePeriodicallyWithCount.c
	node-TestControlAndTelemetry/vuo.test.spinOffWithCount.c
	node-TestControlAndTelemetry/vuo.test.temporarilyDisableTermination.c
	node-TestControlAndTelemetry/vuo.test.writeTimeToFile.c
)
add_dependencies(TestControlAndTelemetry
	TestControlAndTelemetry.nodes
	VuoEditorApp
)
