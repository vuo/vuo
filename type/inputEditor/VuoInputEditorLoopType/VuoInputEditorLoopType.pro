TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorLoopType

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorLoopType.cc

HEADERS += \
		VuoInputEditorLoopType.hh

OTHER_FILES += \
		VuoInputEditorLoopType.json

INCLUDEPATH += \
	$$ROOT/node/vuo.image

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoLoopType.o \
	$$ROOT/type/list/VuoList_VuoLoopType.o
