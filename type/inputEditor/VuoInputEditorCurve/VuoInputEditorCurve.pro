TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorCurve

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorCurve.cc

HEADERS += \
		VuoInputEditorCurve.hh

OTHER_FILES += \
		VuoInputEditorCurve.json

INCLUDEPATH += $$ROOT/node/vuo.math
LIBS += $$ROOT/node/vuo.math/VuoCurve.o
