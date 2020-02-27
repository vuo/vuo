VuoTest(NAME TestVuoEditor
	SOURCE TestVuoEditor.cc
	RESOURCE_LOCK GUI;COMPILER
)
set_target_properties(TestVuoEditor PROPERTIES
	AUTORCC ON
)
target_sources(TestVuoEditor
	PRIVATE
	../../editor/VuoEditorApp/VuoEditorApp.qrc
)
target_link_libraries(TestVuoEditor
	PRIVATE
	VuoEditor
)
add_dependencies(TestVuoEditor VuoEditorApp)

add_test(NAME TestVuoEditorScreenshot
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/runAndScreenshotEditor.sh
)
set_tests_properties(TestVuoEditorScreenshot PROPERTIES
	RESOURCE_LOCK GUI
	FIXTURES_REQUIRED TestScreenshots
)

if (VuoPro)
	include(pro/pro.cmake)
endif()
