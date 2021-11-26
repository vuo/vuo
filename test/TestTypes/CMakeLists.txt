VuoTest(NAME TestTypes
	SOURCE TestTypes.cc
	GMALLOC_ELIGIBLE
)
target_link_libraries(TestTypes
	PRIVATE
	TestCompositionExecution
)
