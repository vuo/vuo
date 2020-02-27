VuoTest(NAME TestReferenceCounting
	SOURCE TestReferenceCounting.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestReferenceCounting
	PRIVATE
	TestCompositionExecution
)

VuoCompileNodes(
	node-TestReferenceCounting/vuo.test.registerTwice.c
	node-TestReferenceCounting/vuo.test.retainWithoutRegister.c
	node-TestReferenceCounting/vuo.test.releaseWithoutRegister.c
	node-TestReferenceCounting/vuo.test.releaseWithoutAnyRetains.c
	node-TestReferenceCounting/vuo.test.releaseWithoutEnoughRetains.c
	node-TestReferenceCounting/vuo.test.retainWithoutRelease.c
	node-TestReferenceCounting/vuo.test.registerOnly.c
	node-TestReferenceCounting/vuo.test.registerNull.c
	node-TestReferenceCounting/vuo.test.retainNull.c
	node-TestReferenceCounting/vuo.test.releaseNull.c
	node-TestReferenceCounting/vuo.test.outputStringAndEvent.c
	node-TestReferenceCounting/vuo.test.storeString.c
	node-TestReferenceCounting/vuo.test.storeStructOfStrings.c
	node-TestReferenceCounting/vuo.test.inputList.c
	node-TestReferenceCounting/vuo.test.outputList.c
	node-TestReferenceCounting/vuo.test.makeListOfSceneObjects.c
)
add_dependencies(TestReferenceCounting TestReferenceCounting.nodes)
