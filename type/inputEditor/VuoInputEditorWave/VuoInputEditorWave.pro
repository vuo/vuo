TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorWave

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorWave.cc

HEADERS += \
	VuoInputEditorWave.hh

OTHER_FILES += \
		VuoInputEditorWave.json

INCLUDEPATH += $$ROOT/node/vuo.math
LIBS += $$ROOT/node/vuo.math/VuoWave.o
