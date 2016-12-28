TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorTransform

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorTransform.cc

HEADERS += \
	VuoInputEditorTransform.hh

OTHER_FILES += \
	VuoInputEditorTransform.json

INCLUDEPATH += $$ROOT/type/inputEditor/VuoInputEditorTransform

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoTransform.o \
	$$ROOT/type/VuoPoint2d.o \
	$$ROOT/type/VuoPoint3d.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/list/VuoList_VuoReal.o \
	$$ROOT/type/VuoText.o
