VuoTest(NAME TestModuleCaches
	SOURCE TestModuleCaches.cc
	FIXTURE TestModuleCaches

	# This test modifies shared caches, which can trip up other tests.
	RUN_SERIAL ON
)
target_link_libraries(TestModuleCaches
	PRIVATE
	TestCompositionExecution
)

target_sources(TestModuleCaches
	PRIVATE
	InstalledModulesChange.cc
	InstalledModulesChange.hh
	ModuleCachesDiff.cc
	ModuleCachesDiff.hh
	ModuleLocation.cc
	ModuleLocation.hh
	ModuleScope.cc
	ModuleScope.hh
)

VuoCompileNodes(
	node-TestModuleCaches/vuo.test.fillRealList.c
	node-TestModuleCaches/vuo.test.selectItems.c
)

add_dependencies(TestModuleCaches
	TestModuleCaches.nodes
	VuoEditorApp
)

# Delete modules installed by this test.
add_test(NAME TestModuleCachesCleanup
	COMMAND rm -f
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.fillRealList.vuoNode
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.makeColorOpaque.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.passThrough.c
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.selectItems.vuoNode
)
set_tests_properties(TestModuleCachesCleanup PROPERTIES
	FIXTURES_CLEANUP TestModuleCaches
)
