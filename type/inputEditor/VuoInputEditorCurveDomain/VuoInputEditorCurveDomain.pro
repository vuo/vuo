TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorCurveDomain

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorCurveDomain.cc

HEADERS += \
	VuoInputEditorCurveDomain.hh

OTHER_FILES += \
	VuoInputEditorCurveDomain.json

INCLUDEPATH += $$ROOT/node/vuo.math
LIBS += $$ROOT/node/vuo.math/VuoCurveDomain.o
