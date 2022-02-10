add_library(VuoRenderer STATIC
	VuoRendererCable.cc
	VuoRendererCable.hh
	VuoRendererColors.cc
	VuoRendererColors.hh
	VuoRendererComment.cc
	VuoRendererComment.hh
	VuoRendererCommon.cc
	VuoRendererCommon.hh
	VuoRendererComposition.cc
	VuoRendererComposition.hh
	VuoRendererFonts.cc
	VuoRendererFonts.hh
	VuoRendererHiddenInputAttachment.cc
	VuoRendererHiddenInputAttachment.hh
	VuoRendererInputAttachment.cc
	VuoRendererInputAttachment.hh
	VuoRendererInputDrawer.cc
	VuoRendererInputDrawer.hh
	VuoRendererInputListDrawer.cc
	VuoRendererInputListDrawer.hh
	VuoRendererItem.cc
	VuoRendererItem.hh
	VuoRendererKeyListForReadOnlyDictionary.cc
	VuoRendererKeyListForReadOnlyDictionary.hh
	VuoRendererNode.cc
	VuoRendererNode.hh
	VuoRendererPort.cc
	VuoRendererPort.hh
	VuoRendererPublishedPort.cc
	VuoRendererPublishedPort.hh
	VuoRendererReadOnlyDictionary.cc
	VuoRendererReadOnlyDictionary.hh
	VuoRendererSignaler.cc
	VuoRendererSignaler.hh
	VuoRendererTypecastPort.cc
	VuoRendererTypecastPort.hh
	VuoRendererValueListForReadOnlyDictionary.cc
	VuoRendererValueListForReadOnlyDictionary.hh
)
set_target_properties(VuoRenderer PROPERTIES
	AUTOMOC ON
)
target_compile_definitions(VuoRenderer
	PUBLIC
	LLVM
	ZMQ
)
target_include_directories(VuoRenderer
	PUBLIC
	.
	../base

	PRIVATE
		${PROJECT_BINARY_DIR}/type/list
		../compiler
		../library
		../node
		../node/vuo.audio
		../node/vuo.file
		../type
		../type/list
)
target_link_libraries(VuoRenderer
	PUBLIC
	objc
	Qt5::Widgets
	CONAN_PKG::jsonc
	CONAN_PKG::libxml2
	${PROJECT_BINARY_DIR}/library/VuoTextHtml.o
	${PROJECT_BINARY_DIR}/node/vuo.file/VuoFileType.o
)
if (VuoPro)
	include(pro/pro.cmake)
endif()
VuoPCH(VuoRenderer LLVMQt)

# Make the Qt frameworks available to vuo-export's relative RPATHs.
file(COPY
		${CONAN_QT_ROOT}/lib/QtCore.framework
		${CONAN_QT_ROOT}/lib/QtGui.framework
		${CONAN_QT_ROOT}/lib/QtSvg.framework
		${CONAN_QT_ROOT}/lib/QtWidgets.framework
		${CONAN_QT_ROOT}/lib/QtPrintSupport.framework
	DESTINATION ${PROJECT_BINARY_DIR}/resources
)
