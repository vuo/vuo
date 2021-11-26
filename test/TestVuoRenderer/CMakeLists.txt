VuoTest(NAME TestVuoRenderer
	SOURCE TestVuoRenderer.cc
	GMALLOC_ELIGIBLE
)
target_link_libraries(TestVuoRenderer
	PUBLIC
	VuoRenderer
)
