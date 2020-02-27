VuoTest(NAME TestTypes
	SOURCE TestTypes.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestTypes
	PRIVATE
	TestCompositionExecution
)
