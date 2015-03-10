TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorThresholdType

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorThresholdType.cc

HEADERS += \
		VuoInputEditorThresholdType.hh

OTHER_FILES += \
		VuoInputEditorThresholdType.json

INCLUDEPATH += $$ROOT/node/vuo.image
LIBS += $$ROOT/node/vuo.image/VuoThresholdType.o
