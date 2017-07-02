TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorArtNetOutputDevice

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorArtNetOutputDevice.cc

HEADERS += \
	VuoInputEditorArtNetOutputDevice.hh

OTHER_FILES += \
	VuoInputEditorArtNetOutputDevice.json

INCLUDEPATH += \
	$$ROOT/node \
	$$ROOT/node/vuo.artnet \
	$$ROOT/node/vuo.artnet/premium \
	$$ROOT/type

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/node/vuo.artnet/premium/VuoArtNet.o \
	$$ROOT/node/vuo.artnet/VuoArtNetInputDevice.o \
	$$ROOT/node/vuo.artnet/VuoArtNetOutputDevice.o \
	$$ROOT/runtime/VuoEventLoop.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/list/VuoList_VuoArtNetInputDevice.o \
	$$ROOT/type/list/VuoList_VuoArtNetOutputDevice.o \
	$$ROOT/type/list/VuoList_VuoInteger.o
