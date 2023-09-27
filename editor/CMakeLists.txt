# Generate `VuoNodeClassFrequencyMap.hh`.
add_custom_command(
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMAND LC_ALL=C ./countNodes.pl ../node -c > ${CMAKE_CURRENT_BINARY_DIR}/VuoNodeClassFrequencyMap.hh
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/VuoNodeClassFrequencyMap.hh
)


add_library(VuoEditor STATIC
	VuoActivityIndicator.cc
	VuoActivityIndicator.hh
	VuoCodeEditor.cc
	VuoCodeEditor.hh
	VuoCodeEditorStages.cc
	VuoCodeEditorStages.hh
	VuoCodeGutter.cc
	VuoCodeGutter.hh
	VuoCodeHighlighterGLSL.cc
	VuoCodeHighlighterGLSL.hh
	VuoCodeIssueList.cc
	VuoCodeIssueList.hh
	VuoCodeWindow.cc
	VuoCodeWindow.hh
	VuoCommandAdd.cc
	VuoCommandAdd.hh
	VuoCommandAddPublishedPort.cc
	VuoCommandAddPublishedPort.hh
	VuoCommandChangeNode.cc
	VuoCommandChangeNode.hh
	VuoCommandCommon.cc
	VuoCommandCommon.hh
	VuoCommandConnect.cc
	VuoCommandConnect.hh
	VuoCommandMove.cc
	VuoCommandMove.hh
	VuoCommandPublishPort.cc
	VuoCommandPublishPort.hh
	VuoCommandRemove.cc
	VuoCommandRemove.hh
	VuoCommandRemoveProtocolPort.cc
	VuoCommandRemoveProtocolPort.hh
	VuoCommandReorderPublishedPorts.cc
	VuoCommandReorderPublishedPorts.hh
	VuoCommandReplaceNode.cc
	VuoCommandReplaceNode.hh
	VuoCommandResizeComment.cc
	VuoCommandResizeComment.hh
	VuoCommandSetCableHidden.cc
	VuoCommandSetCableHidden.hh
	VuoCommandSetCommentText.cc
	VuoCommandSetCommentText.hh
	VuoCommandSetItemTint.cc
	VuoCommandSetItemTint.hh
	VuoCommandSetMetadata.cc
	VuoCommandSetMetadata.hh
	VuoCommandSetNodeTitle.cc
	VuoCommandSetNodeTitle.hh
	VuoCommandSetPortConstant.cc
	VuoCommandSetPortConstant.hh
	VuoCommandSetPublishedPortDetails.cc
	VuoCommandSetPublishedPortDetails.hh
	VuoCommandSetPublishedPortName.cc
	VuoCommandSetPublishedPortName.hh
	VuoCommandSetTriggerThrottling.cc
	VuoCommandSetTriggerThrottling.hh
	VuoCommandUnpublishPort.cc
	VuoCommandUnpublishPort.hh
	VuoCommentEditor.cc
	VuoCommentEditor.hh
	VuoCompositionMetadataPanel.cc
	VuoCompositionMetadataPanel.hh
	VuoConsole.cc
	VuoConsole.hh
	VuoConsoleToolbar.hh
	VuoConsoleToolbar.mm
	VuoConsoleWindow.cc
	VuoConsoleWindow.hh
	VuoDetailsEditorNumeric.cc
	VuoDetailsEditorNumeric.hh
	VuoDetailsEditorPoint.cc
	VuoDetailsEditorPoint.hh
	VuoDialogWithoutTitlebar.hh
	VuoDialogWithoutTitlebar.mm
	VuoDocumentationSidebar.cc
	VuoDocumentationSidebar.hh
	VuoEditor.cc
	VuoEditor.hh
	VuoEditorAboutBox.cc
	VuoEditorAboutBox.hh
	VuoEditorAboutBox.ui
	VuoEditorCocoa.hh
	VuoEditorCocoa.mm
	VuoEditorComposition.cc
	VuoEditorComposition.hh
	VuoEditorGraphicsView.cc
	VuoEditorGraphicsView.hh
	VuoEditorUtilities.cc
	VuoEditorUtilities.hh
	VuoEditorWindow.cc
	VuoEditorWindow.hh
	VuoEditorWindow.ui
	VuoEditorWindowToolbar.hh
	VuoEditorWindowToolbar.mm
	VuoErrorDialog.cc
	VuoErrorDialog.hh
	VuoErrorMark.cc
	VuoErrorMark.hh
	VuoErrorPopover.cc
	VuoErrorPopover.hh
	VuoExampleMenu.cc
	VuoExampleMenu.hh
	VuoInfoDialog.cc
	VuoInfoDialog.hh
	VuoInputEditorManager.cc
	VuoInputEditorManager.hh
	VuoInputEditorSession.cc
	VuoInputEditorSession.hh
	VuoItemDelegate.cc
	VuoItemDelegate.hh
	VuoLineEdit.cc
	VuoLineEdit.hh
	VuoMainWindow.cc
	VuoMainWindow.hh
	VuoMetadataEditor.cc
	VuoMetadataEditor.hh
	VuoMetadataEditor.ui
	VuoModuleManager.cc
	VuoModuleManager.hh
	VuoNodeAndPortIdentifierCache.cc
	VuoNodeAndPortIdentifierCache.hh
	VuoNodeClassFrequencyMap.hh
	VuoNodeClassList.cc
	VuoNodeClassList.hh
	VuoNodeClassListItemDelegate.cc
	VuoNodeClassListItemDelegate.hh
	VuoNodeClassListItemHighlighter.cc
	VuoNodeClassListItemHighlighter.hh
	VuoNodeLibrary.cc
	VuoNodeLibrary.hh
	VuoNodeLibrary.ui
	VuoNodeLibraryTextFilter.cc
	VuoNodeLibraryTextFilter.hh
	VuoNodePopover.cc
	VuoNodePopover.hh
	VuoPanelDocumentation.cc
	VuoPanelDocumentation.hh
	VuoPopover.cc
	VuoPopover.hh
	VuoPortPopover.cc
	VuoPortPopover.hh
	VuoPublishedPortDropBox.cc
	VuoPublishedPortDropBox.hh
	VuoPublishedPortList.cc
	VuoPublishedPortList.hh
	VuoPublishedPortListItemDelegate.cc
	VuoPublishedPortListItemDelegate.hh
	VuoPublishedPortNameEditor.cc
	VuoPublishedPortNameEditor.hh
	VuoPublishedPortSidebar.cc
	VuoPublishedPortSidebar.hh
	VuoPublishedPortSidebar.ui
	VuoRecentFileMenu.cc
	VuoRecentFileMenu.hh
	VuoSearchBox.cc
	VuoSearchBox.hh
	VuoSearchBox.ui
	VuoSliderWithLabels.cc
	VuoSliderWithLabels.hh
	VuoSubcompositionMessageRouter.cc
	VuoSubcompositionMessageRouter.hh
	VuoSubcompositionSaveAsDialog.cc
	VuoSubcompositionSaveAsDialog.hh
	VuoSubcompositionSaveAsDialog.ui
	VuoTitleEditor.cc
	VuoTitleEditor.hh
	VuoToolbar.hh
	VuoToolbar.mm
	VuoToolbarTitleCell.hh
	VuoToolbarTitleCell.mm
)
target_include_directories(VuoEditor
	PUBLIC
		.
		../base
		../compiler
		../renderer

	PRIVATE
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
	Qt5::QuickWidgets
	CONAN_PKG::jsonc
	CONAN_PKG::openssl
	VuoRenderer
	VuoWidget
	${PROJECT_BINARY_DIR}/library/VuoImageResize.o
)

if (VuoPro)
	include(pro/pro.cmake)
endif()

VuoPCH(VuoEditor LLVMQtType)

add_subdirectory(VuoEditorApp)
