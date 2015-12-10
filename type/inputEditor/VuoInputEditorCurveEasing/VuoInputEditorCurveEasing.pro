TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorCurveEasing

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorCurveEasing.cc

HEADERS += \
	VuoInputEditorCurveEasing.hh

OTHER_FILES += \
	VuoInputEditorCurveEasing.json

INCLUDEPATH += \
	$$ROOT/node/vuo.image

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoCurve.o \
	$$ROOT/type/VuoCurveEasing.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/list/VuoList_VuoCurve.o \
	$$ROOT/type/list/VuoList_VuoCurveEasing.o
