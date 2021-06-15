VuoTest(NAME TestEventDropping
	SOURCE TestEventDropping.cc

	# This test installs modules to shared folders, which can trip up other tests.
	RUN_SERIAL ON
	FIXTURE TestEventDropping
)
target_link_libraries(TestEventDropping
	PRIVATE
	TestCompositionExecution
)

VuoCompileNodes(
	node-TestEventDropping/vuo.test.delay.c
)
add_dependencies(TestEventDropping TestEventDropping.nodes)

# Delete modules installed by this test.
add_test(NAME TestEventDroppingCleanup
	COMMAND rm -f
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.fireFromWithin1.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.fireFromWithin1_drop.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.fireFromWithin2.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.fireFromWithin2_drop.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.fireFromWithin3.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.fireFromWithin3_drop.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.fireFromWithin4.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.fireFromWithin4_drop.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.fireFromWithin5.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.fireFromWithin5_drop.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.passthru1.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.passthru2.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.passthru3.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.passthru4.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.passthru5.vuo
)
set_tests_properties(TestEventDroppingCleanup PROPERTIES
	FIXTURES_CLEANUP TestEventDropping
)
