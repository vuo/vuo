VuoTest(NAME TestImageFilters
	SOURCE TestImageFilters.cc
)
target_link_libraries(TestImageFilters
	PRIVATE
	TestCompositionExecution
)
add_dependencies(TestImageFilters
	VuoEditorApp
)
