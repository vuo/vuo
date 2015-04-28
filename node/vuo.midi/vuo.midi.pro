TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.midi.filter.controller.c \
	vuo.midi.filter.note.c \
	vuo.midi.get.controller.c \
	vuo.midi.get.device.c \
	vuo.midi.get.note.c \
	vuo.midi.listDevices.c \
	vuo.midi.make.controller.c \
	vuo.midi.make.device.id.c \
	vuo.midi.make.device.name.c \
	vuo.midi.make.note.c \
	vuo.midi.convert.note.frequency.c \
	vuo.midi.receive.c \
	vuo.midi.send.c

NODE_LIBRARY_SOURCES += \
	VuoMidi.cc

# OTHER_FILES instead of HEADERS, to avoid including in Vuo.framework
OTHER_FILES += \
	VuoMidi.h

NODE_LIBRARY_INCLUDEPATH = \
	$${RTMIDI_ROOT}/include

TYPE_SOURCES += \
	VuoMidiController.c \
	VuoMidiDevice.c \
	VuoMidiNote.c

HEADERS += \
	VuoMidiController.h \
	VuoMidiDevice.h \
	VuoMidiNote.h

include(../../module.pri)
