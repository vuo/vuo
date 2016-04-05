TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorSerialDevice

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorSerialDevice.cc

HEADERS += \
		VuoInputEditorSerialDevice.hh

OTHER_FILES += \
		VuoInputEditorSerialDevice.json

INCLUDEPATH += \
	$$ROOT/node \
	$$ROOT/node/vuo.data \
	$$ROOT/node/vuo.serial \
	$$ROOT/type

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/node/vuo.serial/VuoSerialDevice.o \
	$$ROOT/node/vuo.serial/VuoSerialDevices.o \
	$$ROOT/node/vuo.serial/VuoSerialIO.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/list/VuoList_VuoSerialDevice.o \
	-framework IOKit
