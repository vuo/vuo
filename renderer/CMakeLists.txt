add_library(VuoRenderer STATIC
	VuoRendererCable.cc
	VuoRendererColors.cc
	VuoRendererComment.cc
	VuoRendererCommon.cc
	VuoRendererComposition.cc
	VuoRendererFonts.cc
	VuoRendererHiddenInputAttachment.cc
	VuoRendererInputAttachment.cc
	VuoRendererInputDrawer.cc
	VuoRendererInputListDrawer.cc
	VuoRendererItem.cc
	VuoRendererKeyListForReadOnlyDictionary.cc
	VuoRendererNode.cc
	VuoRendererPort.cc
	VuoRendererPublishedPort.cc
	VuoRendererReadOnlyDictionary.cc
	VuoRendererSignaler.cc
	VuoRendererTypecastPort.cc
	VuoRendererValueListForReadOnlyDictionary.cc
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
		../node/vuo.audio
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
)
if (VuoPro)
	include(pro/pro.cmake)
endif()
VuoPCH(VuoRenderer LLVMQt)

add_subdirectory(vuo-render)

# Make the Qt frameworks available to vuo-export and vuo-render's relative RPATHs.
file(COPY
		${CONAN_QT_ROOT}/lib/QtCore.framework
		${CONAN_QT_ROOT}/lib/QtGui.framework
		${CONAN_QT_ROOT}/lib/QtSvg.framework
		${CONAN_QT_ROOT}/lib/QtWidgets.framework
		${CONAN_QT_ROOT}/lib/QtPrintSupport.framework
	DESTINATION ${PROJECT_BINARY_DIR}/resources
)
