TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorScreen

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorScreen.cc

HEADERS += \
		VuoInputEditorScreen.hh

OTHER_FILES += \
		VuoInputEditorScreen.json

INCLUDEPATH += \
	$$ROOT/type \
	$$ROOT/node/vuo.image

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/library/VuoScreenCommon.bc \
	$$ROOT/type/VuoBoolean.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoPoint2d.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/VuoScreen.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/list/VuoList_VuoBoolean.o \
	$$ROOT/type/list/VuoList_VuoReal.o \
	$$ROOT/type/list/VuoList_VuoScreen.o \
	-framework IOKit \
	-framework AppKit
