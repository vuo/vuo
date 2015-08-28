TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorImageWrapMode

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorImageWrapMode.cc

HEADERS += \
		VuoInputEditorImageWrapMode.hh

OTHER_FILES += \
		VuoInputEditorImageWrapMode.json

INCLUDEPATH += $$ROOT/type
LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoImageWrapMode.o \
	$$ROOT/type/list/VuoList_VuoImageWrapMode.o
