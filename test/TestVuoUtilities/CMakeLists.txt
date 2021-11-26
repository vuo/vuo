VuoTest(NAME TestVuoUtilities
	SOURCE TestVuoUtilities.cc
	GMALLOC_ELIGIBLE
)
add_dependencies(TestVuoUtilities VuoEditorApp)
