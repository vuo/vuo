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
	VuoHorizontalAlignment.c \
	VuoInteger.c \
	VuoImage.c \
	VuoImageWrapMode.c \
	VuoLoopType.c \
	VuoModifierKey.c \
	VuoPoint2d.c \
	VuoPoint3d.c \
	VuoPoint4d.c \
	VuoReal.c \
	VuoSceneObject.c \
	VuoVertices.c \
	VuoShader.c \
	VuoSizingMode.c \
	VuoText.c \
	VuoTransform.c \
	VuoTransform2d.c \
	VuoVerticalAlignment.c \
	VuoWave.c \
	VuoWindowReference.m \
	VuoWrapMode.c

HEADERS += \
	VuoAudioSamples.h \
	VuoBoolean.h \
	VuoBlendMode.h \
	VuoColor.h \
	VuoCurve.h \
	VuoCurveEasing.h \
	VuoHorizontalAlignment.h \
	VuoInteger.h \
	VuoImage.h \
	VuoImageWrapMode.h \
	VuoLoopType.h \
	VuoModifierKey.h \
	VuoPoint2d.h \
	VuoPoint3d.h \
	VuoPoint4d.h \
	VuoReal.h \
	VuoSceneObject.h \
	VuoVertices.h \
	VuoShader.h \
	VuoSizingMode.h \
	VuoText.h \
	VuoTransform.h \
	VuoTransform2d.h \
	VuoVerticalAlignment.h \
	VuoWave.h \
	VuoWindowReference.h \
	VuoWrapMode.h

INCLUDEPATH += \
	$$ROOT/library \
	$$ROOT/node \
	$$ROOT/runtime \
	$$ROOT/type/list

HEADERS += \
	type.h

include(../module.pri)
