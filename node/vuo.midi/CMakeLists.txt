VuoCompileLibraries(
	VuoMidi.cc
)
target_sources(vuo.midi.libraries PRIVATE
	VuoMidi.h
)
target_link_libraries(vuo.midi.libraries
	PUBLIC
	"-framework CoreMIDI"
	CONAN_PKG::rtmidi
	VuoApp
)

VuoCompileTypes(
	VuoMidiController.c
	VuoMidiInputDevice.c
	VuoMidiNote.c
	VuoMidiOutputDevice.c
	VuoMidiPitchBend.c
	VuoNotePriority.c
)
target_sources(vuo.midi.types PRIVATE
	VuoMidiController.h
	VuoMidiInputDevice.h
	VuoMidiNote.h
	VuoMidiOutputDevice.h
	VuoMidiPitchBend.h
	VuoNotePriority.h
)

VuoCompileCompoundTypes(
	# List types referenced by headers in this node set
	VuoList_VuoMidiInputDevice.cc
	VuoList_VuoMidiOutputDevice.cc
	VuoList_VuoNotePriority.cc

	# Additional list types referenced by node classes in this node set
	VuoList_VuoMidiNote.cc
)

VuoCompileNodes(
	vuo.midi.filter.controller.c
	vuo.midi.filter.note.c
	vuo.midi.filter.pitchBend.c
	vuo.midi.get.controller.c
	vuo.midi.get.input.c
	vuo.midi.get.note.c
	vuo.midi.get.output.c
	vuo.midi.get.pitchBend.c
	vuo.midi.listDevices.c
	vuo.midi.listDevices2.c
	vuo.midi.make.controller.c
	vuo.midi.make.input.id.c
	vuo.midi.make.input.name.c
	vuo.midi.make.note.c
	vuo.midi.make.output.id.c
	vuo.midi.make.output.name.c
	vuo.midi.make.pitchBend.c
	vuo.midi.note.frequency.c
	vuo.midi.receive.c
	vuo.midi.send.c
	vuo.midi.smooth.controller.c
	vuo.midi.track.note.mono.c
	vuo.midi.track.note.poly.c
)
target_include_directories(vuo.midi.nodes
	PRIVATE
	../vuo.bcf2000
)

VuoNodeSet()
