VuoTest(NAME TestSubcompositions
	SOURCE TestSubcompositions.cc

	# This test installs modules to shared folders, which can trip up other tests.
	RUN_SERIAL ON
	FIXTURE TestSubcompositions
)
target_link_libraries(TestSubcompositions
	PRIVATE
	VuoRenderer
	TestCompositionExecution
)
add_dependencies(TestSubcompositions VuoEditorApp)

# Delete modules installed by this test.
add_test(NAME TestSubcompositionsCleanup
	COMMAND bash -c "rm -f ~/Library/Application?Support/Vuo/Modules/vuo.test.*.vuo || true"
)
set_tests_properties(TestSubcompositionsCleanup PROPERTIES
	FIXTURES_CLEANUP TestSubcompositions
)
