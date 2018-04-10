TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorColor

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorColor.cc

HEADERS += \
	VuoInputEditorColor.hh

OTHER_FILES += \
		VuoInputEditorColor.json

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoColor.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/list/VuoList_VuoColor.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/list/VuoList_VuoInteger.o \
	$$ROOT/type/list/VuoList_VuoReal.o
