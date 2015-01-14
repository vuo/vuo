TEMPLATE = lib
CONFIG += staticlib icu json
TARGET = VuoType

include(../vuo.pri)

TYPE_SOURCES += \
	VuoBoolean.c \
	VuoColor.c \
	VuoFrameRequest.c \
	VuoInteger.c \
	VuoImage.c \
	VuoMouseButtonAction.c \
	VuoPoint2d.c \
	VuoPoint3d.c \
	VuoPoint4d.c \
	VuoReal.c \
	VuoSceneObject.c \
	VuoVertices.c \
	VuoShader.c \
	VuoText.c \
	VuoTransform.c \
	VuoTransform2d.c

HEADERS += \
	VuoBoolean.h \
	VuoColor.h \
	VuoFrameRequest.h \
	VuoInteger.h \
	VuoImage.h \
	VuoMouseButtonAction.h \
	VuoPoint2d.h \
	VuoPoint3d.h \
	VuoPoint4d.h \
	VuoReal.h \
	VuoSceneObject.h \
	VuoVertices.h \
	VuoShader.h \
	VuoText.h \
	VuoTransform.h \
	VuoTransform2d.h

INCLUDEPATH += \
	$$ROOT/library \
	$$ROOT/node \
	$$ROOT/runtime \
	$$ROOT/type/list \
	$$ICU_ROOT/include

HEADERS += \
	type.h

QMAKE_CLEAN += *.bc

include(../module.pri)
