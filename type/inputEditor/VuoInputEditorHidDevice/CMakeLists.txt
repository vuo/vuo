VuoInputEditor(NAME VuoInputEditorHidDevice SOURCE VuoInputEditorHidDevice.cc)

target_link_libraries(VuoInputEditorHidDevice
	PRIVATE
	"-framework IOKit"
)
