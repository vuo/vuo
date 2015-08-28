TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorHorizontalAlignment

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorHorizontalAlignment.cc

HEADERS += \
		VuoInputEditorHorizontalAlignment.hh

OTHER_FILES += \
		VuoInputEditorHorizontalAlignment.json

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoHorizontalAlignment.o \
	$$ROOT/type/list/VuoList_VuoHorizontalAlignment.o
