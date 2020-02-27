# Generate `VuoNodeClassFrequencyMap.hh`.
add_custom_command(
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMAND LC_ALL=C ./countNodes.pl ../node -c > ${CMAKE_CURRENT_BINARY_DIR}/VuoNodeClassFrequencyMap.hh
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/VuoNodeClassFrequencyMap.hh
)


add_library(VuoEditor STATIC
	VuoActivityIndicator.cc
	VuoCodeEditor.cc
	VuoCodeEditorStages.cc
	VuoCodeGutter.cc
	VuoCodeHighlighterGLSL.cc
	VuoCodeIssueList.cc
	VuoCodeWindow.cc
	VuoCommandAdd.cc
	VuoCommandAddPublishedPort.cc
	VuoCommandCommon.cc
	VuoCommandConnect.cc
	VuoCommandMove.cc
	VuoCommandPublishPort.cc
	VuoCommandRemove.cc
	VuoCommandRemoveProtocolPort.cc
	VuoCommandReorderPublishedPorts.cc
	VuoCommandReplaceNode.cc
	VuoCommandResizeComment.cc
	VuoCommandSetCableHidden.cc
	VuoCommandSetCommentText.cc
	VuoCommandSetItemTint.cc
	VuoCommandSetMetadata.cc
	VuoCommandSetNodeTitle.cc
	VuoCommandSetPortConstant.cc
	VuoCommandSetPublishedPortDetails.cc
	VuoCommandSetPublishedPortName.cc
	VuoCommandSetTriggerThrottling.cc
	VuoCommandChangeNode.cc
	VuoCommandUnpublishPort.cc
	VuoCommentEditor.cc
	VuoCompositionMetadataPanel.cc
	VuoDetailsEditorNumeric.cc
	VuoDetailsEditorPoint.cc
	VuoDialogWithoutTitlebar.mm
	VuoDocumentationSidebar.cc
	VuoEditor.cc
	VuoEditorAboutBox.cc
	VuoEditorAboutBox.ui
	VuoEditorCocoa.mm
	VuoEditorComposition.cc
	VuoEditorGraphicsView.cc
	VuoEditorUtilities.cc
	VuoEditorWindow.cc
	VuoEditorWindow.ui
	VuoEditorWindowToolbar.mm
	VuoErrorDialog.cc
	VuoErrorMark.cc
	VuoErrorPopover.cc
	VuoExampleMenu.cc
	VuoInfoDialog.cc
	VuoInputEditorManager.cc
	VuoInputEditorSession.cc
	VuoItemDelegate.cc
	VuoLineEdit.cc
	VuoMainWindow.cc
	VuoMetadataEditor.cc
	VuoMetadataEditor.ui
	VuoModuleManager.cc
	VuoNodeClassList.cc
	VuoNodeClassListItemDelegate.cc
	VuoNodeClassListItemHighlighter.cc
	VuoNodeLibrary.cc
	VuoNodeLibrary.ui
	VuoNodeLibraryTextFilter.cc
	VuoNodePopover.cc
	VuoPanelDocumentation.cc
	VuoPopover.cc
	VuoPortPopover.cc
	VuoPublishedPortDropBox.cc
	VuoPublishedPortList.cc
	VuoPublishedPortListItemDelegate.cc
	VuoPublishedPortNameEditor.cc
	VuoPublishedPortSidebar.cc
	VuoPublishedPortSidebar.ui
	VuoRecentFileMenu.cc
	VuoSearchBox.cc
	VuoSearchBox.ui
	VuoSliderWithLabels.cc
	VuoSubcompositionMessageRouter.cc
	VuoSubcompositionSaveAsDialog.cc
	VuoSubcompositionSaveAsDialog.ui
	VuoTitleEditor.cc

	VuoNodeClassFrequencyMap.hh
)
target_include_directories(VuoEditor
	PUBLIC
		.
		../base
		../compiler
		../renderer

	PRIVATE
		${PROJECT_BINARY_DIR}/type/list
		../library
		../type

		# For VuoNodeClassFrequencyMap.hh
		${CMAKE_CURRENT_BINARY_DIR}
)
target_compile_definitions(VuoEditor PRIVATE LLVM ZMQ)
set_target_properties(VuoEditor PROPERTIES
	AUTOMOC ON
	AUTOUIC ON
)
target_link_libraries(VuoEditor
	PUBLIC
	"-framework Carbon"
	"-framework Cocoa"
	"-framework IOKit"
	"-framework Security"
	Qt5::Widgets
	Qt5::MacExtras
	Qt5::Network
	Qt5::Quick
	CONAN_PKG::jsonc
	CONAN_PKG::openssl
	VuoRenderer
	VuoWidget
	${PROJECT_BINARY_DIR}/library/VuoImageResize.o
)


# `VUO_NOMINAL_REVISION` is the revision at which cmake was first run in this build folder
# (i.e., when it started from an empty cache).
# This value is cached because the revision changes frequently,
# and when the revision changes that causes the editor to rebuild (because its COMPILE_DEFINITIONS change),
# which takes a while.
if (NOT DEFINED CACHE{VUO_NOMINAL_REVISION})
	set(VUO_NOMINAL_REVISION ${VUO_REVISION} CACHE INTERNAL "")
endif()
set_property(SOURCE VuoEditorAboutBox.cc APPEND PROPERTY COMPILE_DEFINITIONS "VUO_REVISION=\"r${VUO_NOMINAL_REVISION}\"")

if (VuoPro)
	include(pro/pro.cmake)
endif()

VuoPCH(VuoEditor LLVMQt)

add_subdirectory(VuoEditorApp)
