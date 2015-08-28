TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorLeapPointableType

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorLeapPointableType.cc

HEADERS += \
		VuoInputEditorLeapPointableType.hh

OTHER_FILES += \
		VuoInputEditorLeapPointableType.json

INCLUDEPATH += $$ROOT/node/vuo.leap
LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/node/vuo.leap/VuoLeapPointableType.o \
	$$ROOT/type/list/VuoList_VuoLeapPointableType.o
