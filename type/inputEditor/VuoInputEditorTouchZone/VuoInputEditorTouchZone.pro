TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorTouchZone

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorTouchZone.cc

HEADERS += \
		VuoInputEditorTouchZone.hh

OTHER_FILES += \
		VuoInputEditorTouchZone.json

INCLUDEPATH += $$ROOT/node/vuo.leap
LIBS += $$ROOT/node/vuo.leap/VuoLeapTouchZone.o
