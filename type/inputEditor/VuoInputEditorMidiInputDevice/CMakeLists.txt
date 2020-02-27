VuoInputEditor(NAME VuoInputEditorMidiInputDevice SOURCE VuoInputEditorMidiInputDevice.cc)
target_link_libraries(VuoInputEditorMidiInputDevice PRIVATE
	"-framework CoreMIDI"
)
