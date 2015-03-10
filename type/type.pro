TEMPLATE = lib
CONFIG += staticlib icu json
TARGET = VuoType

include(../vuo.pri)

TYPE_SOURCES += \
	VuoBoolean.c \
	VuoBlendMode.c \
	VuoColor.c \
	VuoInteger.c \
	VuoImage.c \
	VuoImageWrapMode.c \
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
	VuoWindowReference.c \
	VuoWrapMode.c

HEADERS += \
	VuoBoolean.h \
	VuoBlendMode.h \
	VuoColor.h \
	VuoInteger.h \
	VuoImage.h \
	VuoImageWrapMode.h \
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
	VuoWindowReference.h \
	VuoWrapMode.h

INCLUDEPATH += \
	$$ROOT/library \
	$$ROOT/node \
	$$ROOT/runtime \
	$$ROOT/type/list \
	$$ICU_ROOT/include

HEADERS += \
	type.h

include(../module.pri)
