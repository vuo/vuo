VuoTest(NAME TestBuiltProducts
	SOURCE TestBuiltProducts.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestBuiltProducts
	PRIVATE
	VuoRenderer
	TestCompositionExecution
)

# Check all the build products.
add_dependencies(TestBuiltProducts
	VuoEditorApp
	vuo-compile
	vuo-compile-for-framework
	vuo-debug
	vuo-export
	vuo-link
)
