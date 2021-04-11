VuoTest(NAME TestProtocolDrivers
	SOURCE TestProtocolDrivers.cc
)
target_link_libraries(TestProtocolDrivers
	PRIVATE
	TestCompositionExecution
)
