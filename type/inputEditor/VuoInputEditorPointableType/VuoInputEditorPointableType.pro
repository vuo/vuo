TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorPointableType

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorPointableType.cc

HEADERS += \
		VuoInputEditorPointableType.hh

OTHER_FILES += \
		VuoInputEditorPointableType.json

INCLUDEPATH += $$ROOT/node/vuo.leap
LIBS += $$ROOT/node/vuo.leap/VuoLeapPointableType.o
