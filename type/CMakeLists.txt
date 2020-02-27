file(GLOB VuoCoreTypeHeaders RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h)
list(FILTER VuoCoreTypeHeaders EXCLUDE REGEX "^(VuoMathExpression|VuoShader(Shaders|Uniforms)|coreTypes|type)\.h$")

VuoCompileTypes(
	VuoAnchor.c
	VuoAudioSamples.c
	VuoBoolean.c
	VuoBlendMode.c
	VuoColor.c
	VuoCoordinateUnit.c
	VuoCubemap.c
	VuoCursor.c
	VuoCurve.c
	VuoCurveEasing.c
	VuoDictionary_VuoText_VuoReal.cc
	VuoDictionary_VuoText_VuoText.cc
	VuoDiode.c
	VuoFont.c
	VuoHorizontalAlignment.c
	VuoInteger.c
	VuoIntegerRange.c
	VuoInteraction.c
	VuoInteractionType.c
	VuoImage.c
	VuoImageColorDepth.c
	VuoImageWrapMode.c
	VuoListPosition.c
	VuoLoopType.c
#	VuoMathExpression.c
	VuoMathExpressionList.c
	VuoMesh.c
	VuoModifierKey.c
	VuoOrientation.c
	VuoPoint2d.c
	VuoPoint3d.c
	VuoPoint4d.c
	VuoRange.c
	VuoReal.c
	VuoRectangle.c
	VuoRenderedLayers.c
	VuoSceneObject.cc
	VuoScreen.c
	VuoShader.cc
	VuoSortOrder.c
	VuoText.c
	VuoTextCase.c
	VuoTextComparison.c
	VuoTextSort.c
	VuoTransform.c
	VuoTransform2d.c
	VuoTree.cc
	VuoUuid.c
	VuoUrl.c
	VuoVerticalAlignment.c
	VuoWave.c
	VuoWindowDescription.c
	VuoWindowProperty.c
	VuoWindowReference.m
	VuoWrapMode.c
)
target_include_directories(vuo.core.types
	PRIVATE
	../base
	../node/vuo.image
	../node/vuo.time
	../node/vuo.ui
)
target_link_libraries(vuo.core.types
	PUBLIC
	"-framework AppKit"
	"-framework OpenGL"
	"-framework IOSurface"
	CONAN_PKG::jsonc
	CONAN_PKG::libxml2
	CONAN_PKG::muparser
	${PROJECT_BINARY_DIR}/library/VuoImageRenderer.o
	${PROJECT_BINARY_DIR}/library/VuoMathExpressionParser.o
	${PROJECT_BINARY_DIR}/library/VuoMeshUtility.o
	${PROJECT_BINARY_DIR}/library/VuoUrlParser.o
	${PROJECT_BINARY_DIR}/library/csgjs.o
	vuo.noise.libraries
	vuo.time.types
)

add_subdirectory(inputEditor)
add_subdirectory(list)
