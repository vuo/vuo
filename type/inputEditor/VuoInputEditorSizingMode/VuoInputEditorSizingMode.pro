TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorSizingMode

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorSizingMode.cc

HEADERS += \
		VuoInputEditorSizingMode.hh

OTHER_FILES += \
		VuoInputEditorSizingMode.json

INCLUDEPATH += \
	$$ROOT/type \
	$$ROOT/node/vuo.image

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/node/vuo.image/VuoSizingMode.o \
	$$ROOT/type/list/VuoList_VuoSizingMode.o
