VuoCompileLibraries(
	VuoClipboard.m
	VuoTextField.cc
	VuoTextFieldInternal.cc
	VuoUiThemeButtonRounded.cc
	VuoUiThemeGroup.cc
	VuoUiThemeSliderRounded.cc
	VuoUiThemeTextFieldRounded.cc
	VuoUiThemeToggleRounded.cc
)
target_sources(vuo.ui.libraries PRIVATE
	VuoClipboard.h
	VuoTextField.h
	VuoTextFieldInternal.h
	VuoUiTheme.h
	VuoUiThemeBase.hh
	material-colors.h
	stb_textedit.h
)
target_include_directories(vuo.ui.libraries
	PRIVATE
	../vuo.layer
)

VuoCompileTypes(
	VuoDragEvent.c
	VuoIconPosition.c
	VuoUiTheme.cc
)
target_sources(vuo.ui.types PRIVATE
	VuoDragEvent.h
	VuoIconPosition.h
	VuoTextEdit.h
)
target_include_directories(vuo.ui.types
	PRIVATE
	../vuo.layer
)
target_link_libraries(vuo.ui.types
	PUBLIC
		${PROJECT_BINARY_DIR}/library/VuoSerializable.o
)

VuoCompileNodes(
	vuo.ui.button.action.c
	vuo.ui.drag.file.c
	vuo.ui.drag.file2.c
	vuo.ui.get.drag.c
#	vuo.ui.get.interactions.c
	vuo.ui.make.slider.cc
	vuo.ui.make.text.c
	vuo.ui.make.textarea.c
	vuo.ui.theme.combine.c
	vuo.ui.make.number.c
#	vuo.ui.make.interaction.c
	vuo.ui.make.toggle.cc
	vuo.ui.make.theme.button.rounded.c
	vuo.ui.make.theme.button.rounded2.c
	vuo.ui.make.theme.material.c
	vuo.ui.make.theme.text.rounded.c
	vuo.ui.make.theme.slider.rounded.c
	vuo.ui.make.theme.toggle.rounded.c
	vuo.ui.make.button.cc
	vuo.ui.open.m
	vuo.ui.save.m
)
target_include_directories(vuo.ui.nodes
	PRIVATE
	../vuo.file
	../vuo.layer
	../vuo.mouse
)

VuoNodeSet()
