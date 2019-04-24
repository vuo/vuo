TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorAudioInputDevice

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorAudioInputDevice.cc

HEADERS += \
		VuoInputEditorAudioInputDevice.hh

OTHER_FILES += \
		VuoInputEditorAudioInputDevice.json

INCLUDEPATH += \
	$$ROOT/node \
	$$ROOT/node/vuo.audio \
	$$ROOT/type

LIBS += \
	$$ROOT/library/libVuoApp.dylib \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/node/vuo.audio/VuoAudio.o \
	$$ROOT/node/vuo.audio/VuoAudioInputDevice.o \
	$$ROOT/node/vuo.audio/VuoAudioOutputDevice.o \
	$$ROOT/type/VuoAudioSamples.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/list/VuoList_VuoAudioInputDevice.o \
	$$ROOT/type/list/VuoList_VuoAudioOutputDevice.o \
	$$ROOT/type/list/VuoList_VuoAudioSamples.o \
	$$ROOT/type/list/VuoList_VuoInteger.o \
	$$ROOT/type/list/VuoList_VuoReal.o \
	$$RTAUDIO_ROOT/lib/librtaudio.a \
	-framework CoreAudio
