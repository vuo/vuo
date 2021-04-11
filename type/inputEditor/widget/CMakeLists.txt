add_library(VuoWidget SHARED
	VuoComboBox.cc
	VuoDialogForInputEditor.cc
	VuoDoubleSpinBox.cc
	VuoInputEditor.cc
	VuoInputEditorCurveRenderer.cc
	VuoInputEditorNamedEnum.cc
	VuoInputEditorWithDialog.cc
	VuoInputEditorWithEnumMenu.cc
	VuoInputEditorWithLineEdit.cc
	VuoInputEditorWithLineEditList.cc
	VuoInputEditorWithMenu.cc
	VuoKeyComboBox.cc
	VuoSpinBox.cc
	VuoTextEditor.cc
)
set_target_properties(VuoWidget PROPERTIES
	AUTOMOC ON

	# Don't automatically add `rpath` search paths to this dylib.
	BUILD_WITH_INSTALL_RPATH TRUE
	INSTALL_RPATH @loader_path/Vuo.framework/Versions/${VuoFrameworkVersion}/Modules
)
target_include_directories(VuoWidget
	PUBLIC
		.
)
target_link_libraries(VuoWidget
	PUBLIC
	Qt5::Widgets
	CONAN_PKG::jsonc
	VuoFrameworkDylib

	PRIVATE
	CONAN_PKG::oscpack
	CONAN_PKG::rtaudio
	CONAN_PKG::rtmidi
	CONAN_PKG::syphon
	${PROJECT_BINARY_DIR}/node/vuo.audio/VuoAudio.o
	${PROJECT_BINARY_DIR}/node/vuo.midi/VuoMidi.o
	${PROJECT_BINARY_DIR}/node/vuo.osc/VuoOscDevices.o
	${PROJECT_BINARY_DIR}/node/vuo.syphon/VuoSyphon.o
	${PROJECT_BINARY_DIR}/node/vuo.syphon/VuoSyphonListener.o
	${PROJECT_BINARY_DIR}/node/vuo.syphon/VuoSyphonSender.o
	${PROJECT_BINARY_DIR}/node/vuo.syphon/VuoSyphonServerNotifier.o
	${PROJECT_BINARY_DIR}/node/vuo.video/VuoVideoCapture.o

	# For unknown reasons, CMake 3.16 duplicates VuoAudio.o and other object files above on the linker command line,
	# unless we list the frameworks _after_ the objects.
	"-framework AVFoundation"
	"-framework CFNetwork"
	"-framework CoreAudio"
	"-framework CoreMIDI"
	"-framework CoreMedia"
	"-framework CoreMediaIO"
	"-framework CoreVideo"
	"-framework Foundation"
	"-framework IOKit"
	"-framework OpenGL"
)
VuoPCH(VuoWidget Qt)
add_custom_command(TARGET VuoWidget POST_BUILD
	COMMAND install_name_tool -change @loader_path/../Frameworks/Syphon.framework/Versions/A/Syphon @rpath/../Frameworks/Vuo.framework/Frameworks/Syphon.framework/Syphon ${PROJECT_BINARY_DIR}/lib/libVuoWidget.dylib
)
