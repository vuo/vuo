TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorHidDevice

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorHidDevice.cc

HEADERS += \
		VuoInputEditorHidDevice.hh

OTHER_FILES += \
		VuoInputEditorHidDevice.json

INCLUDEPATH += \
	$$ROOT/node \
	$$ROOT/node/vuo.hid \
	$$ROOT/type

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/library/VuoIoReturn.o \
	$$ROOT/node/vuo.hid/VuoHidControl.o \
	$$ROOT/node/vuo.hid/VuoHidDevice.o \
	$$ROOT/node/vuo.hid/VuoHidDevices.o \
	$$ROOT/node/vuo.hid/VuoHidIo.o \
	$$ROOT/node/vuo.hid/VuoHidUsage.o \
	$$ROOT/node/vuo.hid/VuoUsbVendor.o \
	$$ROOT/runtime/VuoEventLoop.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/list/VuoList_VuoHidControl.o \
	$$ROOT/type/list/VuoList_VuoHidDevice.o \
	-framework IOKit
