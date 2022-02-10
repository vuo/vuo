set_property(GLOBAL PROPERTY VuoInputEditors)

add_custom_target(VuoInputEditors)

# Adds steps to build an input editor.
function (VuoInputEditor)
	cmake_parse_arguments(arg "" "NAME;SOURCE" "" ${ARGV})
	string(REGEX REPLACE "\.(cc|mm)$" ".hh" header ${arg_SOURCE})

	add_library(${arg_NAME} SHARED ${arg_SOURCE} ${header})
	set_target_properties(${arg_NAME} PROPERTIES
		AUTOMOC ON

		# Don't automatically add `rpath` search paths to this dylib.
		BUILD_WITH_INSTALL_RPATH TRUE
		INSTALL_RPATH @loader_path/../../Frameworks/Vuo.framework/Versions/${VuoFrameworkVersion}/Modules
	)
	target_link_libraries(${arg_NAME}
		PRIVATE
		CONAN_PKG::libxml2
		VuoHeap
		VuoWidget
	)
	VuoPCH(${arg_NAME} Qt)

	add_dependencies(VuoInputEditors ${arg_NAME})

	get_property(VuoInputEditors GLOBAL PROPERTY VuoInputEditors)
	list(APPEND VuoInputEditors ${arg_NAME})
	set_property(GLOBAL PROPERTY VuoInputEditors "${VuoInputEditors}")
endfunction()


add_subdirectory(widget)

add_subdirectory(VuoInputEditorAnchor)
add_subdirectory(VuoInputEditorAudioInputDevice)
add_subdirectory(VuoInputEditorAudioOutputDevice)
add_subdirectory(VuoInputEditorBlendMode)
add_subdirectory(VuoInputEditorColor)
add_subdirectory(VuoInputEditorCurve)
add_subdirectory(VuoInputEditorCurveEasing)
add_subdirectory(VuoInputEditorDispersion)
add_subdirectory(VuoInputEditorDisplacement)
add_subdirectory(VuoInputEditorEdgeBlend)
add_subdirectory(VuoInputEditorFont)
add_subdirectory(VuoInputEditorHidDevice)
add_subdirectory(VuoInputEditorInteger)
add_subdirectory(VuoInputEditorIntegerRange)
add_subdirectory(VuoInputEditorKey)
add_subdirectory(VuoInputEditorLoopType)
add_subdirectory(VuoInputEditorMathExpressionList)
add_subdirectory(VuoInputEditorMidiInputDevice)
add_subdirectory(VuoInputEditorMidiOutputDevice)
add_subdirectory(VuoInputEditorMovieFormat)
add_subdirectory(VuoInputEditorOscInputDevice)
add_subdirectory(VuoInputEditorOscOutputDevice)
add_subdirectory(VuoInputEditorPoint2d)
add_subdirectory(VuoInputEditorPoint3d)
add_subdirectory(VuoInputEditorPoint4d)
add_subdirectory(VuoInputEditorRange)
add_subdirectory(VuoInputEditorReal)
add_subdirectory(VuoInputEditorRealRegulation)
add_subdirectory(VuoInputEditorScreen)
add_subdirectory(VuoInputEditorSerialDevice)
add_subdirectory(VuoInputEditorSizingMode)
add_subdirectory(VuoInputEditorSyphonServerDescription)
add_subdirectory(VuoInputEditorText)
add_subdirectory(VuoInputEditorTextComparison)
add_subdirectory(VuoInputEditorTransform)
add_subdirectory(VuoInputEditorTransform2d)
add_subdirectory(VuoInputEditorVideoInputDevice)
add_subdirectory(VuoInputEditorWave)
add_subdirectory(VuoInputEditorWrapMode)

if (VuoPro)
	include(pro/pro.cmake)
endif()
