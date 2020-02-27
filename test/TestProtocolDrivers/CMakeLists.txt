VuoTest(NAME TestProtocolDrivers
	SOURCE TestProtocolDrivers.cc
	RESOURCE_LOCK GUI;COMPILER
)
target_link_libraries(TestProtocolDrivers
	PRIVATE
	TestCompositionExecution
)
