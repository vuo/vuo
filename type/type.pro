TEMPLATE = lib
CONFIG += staticlib json
TARGET = VuoType

include(../vuo.pri)

TYPE_SOURCES += \
	VuoAnchor.c \
	VuoAudioSamples.c \
	VuoBoolean.c \
	VuoBlendMode.c \
	VuoColor.c \
	VuoCoordinateUnit.c \
	VuoCursor.c \
	VuoCurve.c \
	VuoCurveEasing.c \
	VuoDictionary_VuoText_VuoReal.cc \
	VuoDictionary_VuoText_VuoText.cc \
	VuoDiode.c \
	VuoFont.c \
	VuoHorizontalAlignment.c \
	VuoInteger.c \
	VuoIntegerRange.c \
	VuoImage.c \
	VuoImageColorDepth.c \
	VuoImageWrapMode.c \
	VuoListPosition.c \
	VuoLoopType.c \
#	VuoMathExpression.c \
	VuoMathExpressionList.c \
	VuoMesh.c \
	VuoModifierKey.c \
	VuoPoint2d.c \
	VuoPoint3d.c \
	VuoPoint4d.c \
	VuoRange.c \
	VuoReal.c \
	VuoSceneObject.cc \
	VuoScreen.c \
	VuoShader.cc \
	VuoSortOrder.c \
	VuoText.c \
	VuoTextCase.c \
	VuoTextComparison.c \
	VuoTextSort.c \
	VuoTransform.c \
	VuoTransform2d.c \
	VuoTree.cc \
	VuoUrl.c \
	VuoVerticalAlignment.c \
	VuoWave.c \
	VuoWindowProperty.c \
	VuoWindowReference.m \
	VuoWrapMode.c

TYPE_HEADERS = \
	VuoAnchor.h \
	VuoAudioSamples.h \
	VuoBoolean.h \
	VuoBlendMode.h \
	VuoColor.h \
	VuoCoordinateUnit.h \
	VuoCursor.h \
	VuoCurve.h \
	VuoCurveEasing.h \
	VuoDictionary_VuoText_VuoReal.h \
	VuoDictionary_VuoText_VuoText.h \
	VuoDiode.h \
	VuoFont.h \
	VuoHorizontalAlignment.h \
	VuoInteger.h \
	VuoIntegerRange.h \
	VuoImage.h \
	VuoImageColorDepth.h \
	VuoImageWrapMode.h \
	VuoListPosition.h \
	VuoLoopType.h \
#	VuoMathExpression.h \
	VuoMathExpressionList.h \
	VuoMesh.h \
	VuoModifierKey.h \
	VuoPoint2d.h \
	VuoPoint3d.h \
	VuoPoint4d.h \
	VuoRange.h \
	VuoReal.h \
	VuoSceneObject.h \
	VuoScreen.h \
	VuoShader.h \
	VuoSortOrder.h \
	VuoText.h \
	VuoTextCase.h \
	VuoTextComparison.h \
	VuoTextSort.h \
	VuoTransform.h \
	VuoTransform2d.h \
	VuoTree.h \
	VuoUrl.h \
	VuoVerticalAlignment.h \
	VuoWave.h \
	VuoWindowProperty.h \
	VuoWindowReference.h \
	VuoWrapMode.h

# OTHER_FILES so these headers don't get included in framework/Vuo.h.
OTHER_FILES += \
	VuoShaderShaders.h \
	VuoShaderUniforms.h

INCLUDEPATH += \
	$$ROOT/library \
	$$ROOT/node \
	$$ROOT/node/vuo.data \
	$$ROOT/node/vuo.text \
	$$ROOT/runtime \
	$$ROOT/type/list

TYPE_INCLUDEPATH += \
	../node/vuo.data \
	../node/vuo.text \
	$${LIBXML2_ROOT}/include/libxml2

HEADERS += \
	$$TYPE_HEADERS \
	type.h

exists(coreTypes.h) HEADERS += coreTypes.h

include(../module.pri)
