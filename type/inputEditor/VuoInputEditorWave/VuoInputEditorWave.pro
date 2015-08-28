TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorWave

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorWave.cc

HEADERS += \
	VuoInputEditorWave.hh

OTHER_FILES += \
		VuoInputEditorWave.json

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoWave.o \
	$$ROOT/type/list/VuoList_VuoWave.o
