TEMPLATE = lib
CONFIG += no_link target_predeps staticlib VuoNoLibrary

include(../../../vuo.pri)

NODE_SOURCES += \
	vuo.test.registerTwice.c \
	vuo.test.retainWithoutRegister.c \
	vuo.test.releaseWithoutRegister.c \
	vuo.test.releaseWithoutAnyRetains.c \
	vuo.test.releaseWithoutEnoughRetains.c \
	vuo.test.retainWithoutRelease.c \
	vuo.test.registerOnly.c \
	vuo.test.registerNull.c \
	vuo.test.retainNull.c \
	vuo.test.releaseNull.c \
	vuo.test.outputStringAndEvent.c \
	vuo.test.storeString.c \
	vuo.test.storeStructOfStrings.c \
	vuo.test.inputList.c \
	vuo.test.outputList.c \
	vuo.test.makeListOfSceneObjects.c

OTHER_FILES += $$NODE_SOURCES
