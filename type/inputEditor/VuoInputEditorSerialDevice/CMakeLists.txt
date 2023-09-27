VuoInputEditor(NAME VuoInputEditorSerialDevice SOURCE VuoInputEditorSerialDevice.cc)

target_link_libraries(VuoInputEditorSerialDevice
	PRIVATE
	"-framework IOKit"
)
