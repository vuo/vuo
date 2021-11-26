VuoTest(NAME TestModules
	SOURCE TestModules.cc
	GMALLOC_ELIGIBLE
)
target_link_libraries(TestModules
	PRIVATE
	CONAN_PKG::wjelement
)
add_dependencies(TestModules VuoEditorApp)
