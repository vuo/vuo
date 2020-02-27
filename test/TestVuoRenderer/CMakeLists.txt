VuoTest(NAME TestVuoRenderer
	SOURCE TestVuoRenderer.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestVuoRenderer
	PUBLIC
	VuoRenderer
)
