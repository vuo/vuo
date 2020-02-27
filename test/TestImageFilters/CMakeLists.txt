VuoTest(NAME TestImageFilters
	SOURCE TestImageFilters.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestImageFilters
	PRIVATE
	TestCompositionExecution
)
add_dependencies(TestImageFilters
	VuoEditorApp
)
