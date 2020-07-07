VuoTest(NAME TestModules
	SOURCE TestModules.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestModules
	PRIVATE
	CONAN_PKG::wjelement
)
add_dependencies(TestModules VuoEditorApp)
