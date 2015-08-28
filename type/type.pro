TEMPLATE = lib
CONFIG += staticlib json
TARGET = VuoType

include(../vuo.pri)

TYPE_SOURCES += \
	VuoAudioSamples.c \
	VuoBoolean.c \
	VuoBlendMode.c \
	VuoColor.c \
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
	VuoShader.c \
	VuoSizingMode.c \
	VuoText.c \
	VuoTransform.c \
	VuoTransform2d.c \
	VuoVerticalAlignment.c \
	VuoWave.c \
	VuoWindowProperty.c \
	VuoWindowReference.m \
	VuoWrapMode.c

HEADERS += \
	VuoAudioSamples.h \
	VuoBoolean.h \
	VuoBlendMode.h \
	VuoColor.h \
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
	VuoSizingMode.h \
	VuoText.h \
	VuoTransform.h \
	VuoTransform2d.h \
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
	$$ROOT/runtime \
	$$ROOT/type/list

HEADERS += \
	type.h

include(../module.pri)
