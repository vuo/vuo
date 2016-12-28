TEMPLATE = lib
CONFIG += staticlib json
TARGET = VuoType

include(../vuo.pri)

TYPE_SOURCES += \
	VuoAudioSamples.c \
	VuoBoolean.c \
	VuoBlendMode.c \
	VuoColor.c \
	VuoCoordinateUnit.c \
	VuoCursor.c \
	VuoCurve.c \
	VuoCurveEasing.c \
	VuoDictionary_VuoText_VuoReal.c \
	VuoHorizontalAlignment.c \
	VuoInteger.c \
	VuoImage.c \
	VuoImageColorDepth.c \
	VuoImageWrapMode.c \
	VuoLoopType.c \
#	VuoMathExpression.c \
	VuoMathExpressionList.c \
	VuoMesh.c \
	VuoModifierKey.c \
	VuoPoint2d.c \
	VuoPoint3d.c \
	VuoPoint4d.c \
	VuoReal.c \
	VuoSceneObject.c \
	VuoScreen.c \
	VuoShader.cc \
	VuoText.c \
	VuoTransform.c \
	VuoTransform2d.c \
	VuoUrl.c \
	VuoVerticalAlignment.c \
	VuoWave.c \
	VuoWindowProperty.c \
	VuoWindowReference.m \
	VuoWrapMode.c

TYPE_HEADERS = \
	VuoAudioSamples.h \
	VuoBoolean.h \
	VuoBlendMode.h \
	VuoColor.h \
	VuoCoordinateUnit.h \
	VuoCursor.h \
	VuoCurve.h \
	VuoCurveEasing.h \
	VuoDictionary_VuoText_VuoReal.h \
	VuoHorizontalAlignment.h \
	VuoInteger.h \
	VuoImage.h \
	VuoImageColorDepth.h \
	VuoImageWrapMode.h \
	VuoLoopType.h \
#	VuoMathExpression.h \
	VuoMathExpressionList.h \
	VuoMesh.h \
	VuoModifierKey.h \
	VuoPoint2d.h \
	VuoPoint3d.h \
	VuoPoint4d.h \
	VuoReal.h \
	VuoSceneObject.h \
	VuoScreen.h \
	VuoShader.h \
	VuoText.h \
	VuoTransform.h \
	VuoTransform2d.h \
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
	$$ROOT/node/vuo.font \
	$$ROOT/runtime \
	$$ROOT/type/list

TYPE_INCLUDEPATH += \
	../node/vuo.data \
	../node/vuo.font

HEADERS += \
	$$TYPE_HEADERS \
	type.h

exists(coreTypes.h) HEADERS += coreTypes.h coreTypesStringify.h coreTypesStringify.hh

include(../module.pri)
