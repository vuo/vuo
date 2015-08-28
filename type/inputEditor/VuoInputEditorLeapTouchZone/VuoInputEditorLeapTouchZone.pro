TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorLeapTouchZone

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorLeapTouchZone.cc

HEADERS += \
		VuoInputEditorLeapTouchZone.hh

OTHER_FILES += \
		VuoInputEditorLeapTouchZone.json

INCLUDEPATH += $$ROOT/node/vuo.leap
LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/node/vuo.leap/VuoLeapTouchZone.o \
	$$ROOT/type/list/VuoList_VuoLeapTouchZone.o
