TEMPLATE = lib
CONFIG += staticlib icu json
TARGET = VuoType

include(../vuo.pri)

TYPE_SOURCES += \
	VuoColor.c \
	VuoFrameRequest.c \
	VuoInteger.c \
	VuoImage.c \
	VuoMidiController.c \
	VuoMidiDevice.c \
	VuoMidiNote.c \
	VuoPoint2d.c \
	VuoPoint3d.c \
	VuoPoint4d.c \
	VuoReal.c \
	VuoSceneObject.c \
	VuoVertices.c \
	VuoShader.c \
	VuoText.c \
	VuoTransform.c \
	VuoGradientNoise.c \
	VuoBoolean.c \
	VuoCountWrapMode.c \
	VuoBlendMode.c \
	VuoWave.c \
	VuoNoise.c \
	VuoCurve.c \
	VuoCurveDomain.c \
	VuoMouseButtonAction.c \
	VuoLeapPointable.c \
	VuoLeapFrame.c \
	VuoLeapPointableType.c \
	VuoLeapHand.c

OTHER_FILES += $$TYPE_SOURCES

HEADERS += \
	VuoColor.h \
	VuoFrameRequest.h \
	VuoInteger.h \
	VuoImage.h \
	VuoMidiController.h \
	VuoMidiDevice.h \
	VuoMidiNote.h \
	VuoPoint2d.h \
	VuoPoint3d.h \
	VuoPoint4d.h \
	VuoReal.h \
	VuoSceneObject.h \
	VuoVertices.h \
	VuoShader.h \
	VuoText.h \
	VuoTransform.h \
	VuoGradientNoise.h \
	VuoBoolean.h \
	VuoCountWrapMode.h \
	VuoBlendMode.h \
	VuoWave.h \
	VuoNoise.h \
	VuoCurve.h \
	VuoCurveDomain.h \
	VuoMouseButtonAction.h \
	VuoLeapPointable.h \
	VuoLeapFrame.h \
	VuoLeapPointableType.h \
	VuoLeapHand.h

INCLUDEPATH += \
	$$ROOT/node \
	$$ROOT/runtime \
	$$ROOT/type/list \
	$$ICU_ROOT/include

OTHER_FILES += \
	type.h

QMAKE_CLEAN += *.bc

include(type.pri)

typeObjects.input = TYPE_OBJECTS
typeObjects.output = ${QMAKE_FILE_IN_BASE}.o
typeObjects.commands = $$QMAKE_CC -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
typeObjects.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += typeObjects
