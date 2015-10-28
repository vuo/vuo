TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorCurve

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorCurve.cc

HEADERS += \
		VuoInputEditorCurve.hh \
		VuoInputEditorCurveRenderer.hh

OTHER_FILES += \
		VuoInputEditorCurve.json

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoCurve.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/list/VuoList_VuoCurve.o
