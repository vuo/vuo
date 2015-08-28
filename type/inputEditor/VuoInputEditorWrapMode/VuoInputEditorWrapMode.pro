TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorWrapMode

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorWrapMode.cc

HEADERS += \
		VuoInputEditorWrapMode.hh

OTHER_FILES += \
		VuoInputEditorWrapMode.json

INCLUDEPATH += $$ROOT/type
LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoWrapMode.o \
	$$ROOT/type/list/VuoList_VuoWrapMode.o
