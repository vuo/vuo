VuoTest(NAME TestNodes
	SOURCE TestNodes.cc
	RESOURCE_LOCK GUI;COMPILER
	COST 100
	FIXTURE TestNodes
)
target_link_libraries(TestNodes
	PRIVATE
	VuoEditor
	TestCompositionExecution
)
add_dependencies(TestNodes VuoEditorApp)

# Terminate apps launched by TestNodes, since they interfere with GUI tests.
# Delete files created by executing nodes with their default values.
add_test(NAME TestNodesCleanup
	COMMAND bash -c "pkill -f Calculator.app Safari.app TextEdit.app || true ; rm -Rf ~/Desktop/MyData.txt ~/Desktop/recording.json ~/Desktop/MyFolder || true"
)
set_tests_properties(TestNodesCleanup PROPERTIES
	FIXTURES_CLEANUP TestNodes
)
