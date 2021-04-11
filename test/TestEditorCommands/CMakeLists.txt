VuoTest(NAME TestEditorCommands
	SOURCE TestEditorCommands.cc
)
set_target_properties(TestEditorCommands PROPERTIES
	AUTORCC ON
)
target_sources(TestEditorCommands
	PRIVATE
		../../editor/VuoEditorApp/VuoEditorApp.qrc
)
target_link_libraries(TestEditorCommands
	PRIVATE
		VuoEditor
)
add_dependencies(TestEditorCommands VuoEditorApp)

if (VuoPro)
	include(pro/pro.cmake)
endif()
