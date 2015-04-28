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

LIBS += $$ROOT/type/VuoCurve.o
