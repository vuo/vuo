TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorOscOutputDevice

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorOscOutputDevice.cc

HEADERS += \
		VuoInputEditorOscOutputDevice.hh

OTHER_FILES += \
		VuoInputEditorOscOutputDevice.json

INCLUDEPATH += \
	$$ROOT/node \
	$$ROOT/node/vuo.osc \
	$$ROOT/type

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/node/vuo.osc/VuoOscDevices.o \
	$$ROOT/node/vuo.osc/VuoOscInputDevice.o \
	$$ROOT/node/vuo.osc/VuoOscOutputDevice.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/list/VuoList_VuoOscInputDevice.o \
	$$ROOT/type/list/VuoList_VuoOscOutputDevice.o \
	-framework IOKit
