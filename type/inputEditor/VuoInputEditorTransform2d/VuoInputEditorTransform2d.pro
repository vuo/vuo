TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorTransform2d

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorTransform2d.cc

HEADERS += \
	VuoInputEditorTransform2d.hh

OTHER_FILES += \
	VuoInputEditorTransform2d.json

INCLUDEPATH += $$ROOT/type/inputEditor/VuoInputEditorTransform2d

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoTransform2d.o \
	$$ROOT/type/VuoPoint2d.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/list/VuoList_VuoReal.o \
	$$ROOT/type/VuoText.o
