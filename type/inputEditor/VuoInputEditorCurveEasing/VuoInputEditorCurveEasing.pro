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

LIBS += \
	$$ROOT/type/VuoCurve.o \
	$$ROOT/type/VuoCurveEasing.o
