TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorAnchor

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorAnchor.cc

HEADERS += \
	VuoInputEditorAnchor.hh

OTHER_FILES += \
	VuoInputEditorAnchor.json

INCLUDEPATH += \
	$$ROOT/type

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoAnchor.o \
	$$ROOT/type/VuoHorizontalAlignment.o \
	$$ROOT/type/VuoVerticalAlignment.o \
	$$ROOT/type/list/VuoList_VuoHorizontalAlignment.o \
	$$ROOT/type/list/VuoList_VuoVerticalAlignment.o \
	$$ROOT/type/list/VuoList_VuoAnchor.o
