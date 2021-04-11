set(types
	VuoAudioSamples
	VuoBoolean
	VuoColor
	VuoFont
	VuoInteger
	VuoImage
	VuoLayer
	VuoList
	VuoMathExpression
	VuoMesh
	VuoMidiNote
	VuoPoint2d
	VuoPoint3d
	VuoPoint4d
	VuoReal
	VuoRectangle
	VuoSceneObject
	VuoShader
	VuoTable
	VuoText
	VuoTime
	VuoTransform
	VuoTransform2d
	VuoTree
	VuoUuid
	VuoUiTheme
	VuoUrl
)

if (VuoPro)
	include(pro/pro.cmake)
endif()

foreach (type ${types})
	VuoTest(NAME Test${type} SOURCE Test${type}.cc)
	add_dependencies(Test${type} VuoEditorApp)
endforeach()

target_link_libraries(TestVuoColor
	PRIVATE
		"-framework AppKit"
		vuo.color.libraries
)
target_link_libraries(TestVuoUiTheme
	PRIVATE
	vuo.ui.libraries
)
target_link_libraries(TestVuoUrl
	PRIVATE
		Qt5::Widgets
)
