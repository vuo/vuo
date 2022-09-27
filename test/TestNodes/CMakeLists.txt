VuoTest(NAME TestNodes
	SOURCE TestNodes.cc
	FIXTURE TestNodes
	TIMEOUT 300
)
target_link_libraries(TestNodes
	PRIVATE
	VuoEditor
	TestCompositionExecution
)
add_dependencies(TestNodes VuoEditorApp)

# Terminate apps launched by TestNodes, since they interfere with GUI tests.
# Close Finder windows that were opened by TestNodes (e.g. vuo.app.openDocument), since they interfere with GUI tests.
# Delete files created by executing nodes with their default values.
add_test(NAME TestNodesCleanup
	COMMAND bash -c "pkill -f Calculator.app Safari.app TextEdit.app || true ; osascript -e 'tell application \"Finder\" to close every window' || true ; rm -Rf ~/Desktop/MyData.txt ~/Desktop/recording.json ~/Desktop/MyFolder || true"
)
set_tests_properties(TestNodesCleanup PROPERTIES
	FIXTURES_CLEANUP TestNodes
)
