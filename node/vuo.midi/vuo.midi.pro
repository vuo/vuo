TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.midi.filter.controller.c \
	vuo.midi.filter.note.c \
	vuo.midi.filter.pitchBend.c \
	vuo.midi.get.controller.c \
	vuo.midi.get.input.c \
	vuo.midi.get.output.c \
	vuo.midi.get.note.c \
	vuo.midi.get.pitchBend.c \
	vuo.midi.listDevices.c \
	vuo.midi.make.controller.c \
	vuo.midi.make.input.id.c \
	vuo.midi.make.input.name.c \
	vuo.midi.make.output.id.c \
	vuo.midi.make.output.name.c \
	vuo.midi.make.note.c \
	vuo.midi.make.pitchBend.c \
	vuo.midi.note.frequency.c \
	vuo.midi.receive.c \
	vuo.midi.send.c \
	vuo.midi.smooth.controller.c \
	vuo.midi.track.note.mono.c \
	vuo.midi.track.note.poly.c

NODE_LIBRARY_SOURCES += \
	VuoMidi.cc

HEADERS += \
	VuoMidi.h \
	VuoMidiController.h \
	VuoMidiInputDevice.h \
	VuoMidiNote.h \
	VuoMidiOutputDevice.h \
	VuoMidiPitchBend.h \
	VuoNotePriority.h

NODE_INCLUDEPATH = \
	../vuo.bcf2000 \

NODE_LIBRARY_INCLUDEPATH = \
	$${RTMIDI_ROOT}/include

TYPE_SOURCES += \
	VuoMidiController.c \
	VuoMidiInputDevice.c \
	VuoMidiOutputDevice.c \
	VuoMidiNote.c \
	VuoMidiPitchBend.c \
	VuoNotePriority.c

include(../../module.pri)
