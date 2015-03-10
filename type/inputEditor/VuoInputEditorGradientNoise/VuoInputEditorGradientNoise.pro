TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorGradientNoise

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorGradientNoise.cc

HEADERS += \
	VuoInputEditorGradientNoise.hh

OTHER_FILES += \
	VuoInputEditorGradientNoise.json

INCLUDEPATH += $$ROOT/node/vuo.noise
LIBS += $$ROOT/node/vuo.noise/VuoGradientNoise.o
