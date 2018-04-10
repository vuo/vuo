TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorMidiOutputDevice

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorMidiOutputDevice.cc

HEADERS += \
	VuoInputEditorMidiOutputDevice.hh

OTHER_FILES += \
	VuoInputEditorMidiOutputDevice.json

INCLUDEPATH += \
	$$ROOT/node \
	$$ROOT/node/vuo.midi \
	$$ROOT/type

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/library/VuoOsStatus.o \
	$$ROOT/node/vuo.midi/VuoMidi.o \
	$$ROOT/node/vuo.midi/VuoMidiInputDevice.o \
	$$ROOT/node/vuo.midi/VuoMidiOutputDevice.o \
	$$ROOT/node/vuo.midi/VuoMidiPitchBend.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/list/VuoList_VuoInteger.o \
	$$ROOT/type/list/VuoList_VuoMidiInputDevice.o \
	$$ROOT/type/list/VuoList_VuoMidiOutputDevice.o \
	-L$$RTMIDI_ROOT/lib -lRtMidi \
	-framework CoreAudio \
	-framework CoreMIDI
