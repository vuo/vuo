TEMPLATE = lib
CONFIG += staticlib icu json
TARGET = VuoTypeList

include(../../vuo.pri)

OTHER_FILES += \
	VuoList.cc \
	VuoList.h


# Generate the list type variants, and build the generated list type variants.
# The number after the colon means:
#    0 — type is not reference counted
#    1 — type is reference counted, using VuoRetain() and VuoRelease()
#    2 — type is reference counted, using ELEMENT_TYPE_retain() and ELEMENT_TYPE_release()
TYPE_LIST_SOURCES = $$system( ./generateVariants.sh \
		VuoBlendMode:0 \
		VuoBoolean:0 \
		VuoColor:0 \
		VuoCountWrapMode:0 \
		VuoCurve:0 \
		VuoCurveDomain:0 \
		VuoFrameRequest:0 \
		VuoGradientNoise:0 \
		VuoImage:1 \
		VuoInteger:0 \
		VuoLayer:2 \
		VuoLeapFrame:2 \
		VuoLeapHand:2 \
		VuoLeapPointable:0 \
		VuoLeapPointableType:0 \
		VuoLoopType:0 \
		VuoMidiController:0 \
		VuoMidiDevice:2 \
		VuoMidiNote:0 \
		VuoMouseButtonAction:0 \
		VuoNoise:0 \
		VuoPoint2d:0 \
		VuoPoint3d:0 \
		VuoPoint4d:0 \
		VuoReal:0 \
		VuoSceneObject:2 \
		VuoShader:1 \
		VuoSyphonServerDescription:0 \
		VuoText:1 \
		VuoTransform:0 \
		VuoTransform2d:0 \
		VuoVertices:2 \
		VuoWave:0 \
	)

TYPE_INCLUDEPATH = \
	$$ROOT/node/vuo.image \
	$$ROOT/node/vuo.layer \
	$$ROOT/node/vuo.leap \
	$$ROOT/node/vuo.math \
	$$ROOT/node/vuo.midi \
	$$ROOT/node/vuo.movie \
	$$ROOT/node/vuo.noise \
	$$ROOT/node/vuo.syphon

include(../../module.pri)

typeList.input = TYPE_LIST_SOURCES
typeList.output = ${QMAKE_FILE_IN_BASE}.bc
typeList.depends = $$OTHER_FILES ../*.h
typeList.commands  = $$VUOCOMPILE $$VUOCOMPILE_TYPE_FLAGS --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
typeList.variable_out = TYPE_LIST_OBJECTS
typeList.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += typeList

typeListObjects.input = TYPE_LIST_OBJECTS
typeListObjects.output = ${QMAKE_FILE_IN_BASE}.o
typeListObjects.commands = $$QMAKE_CC -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
typeListObjects.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += typeListObjects

OTHER_FILES = $$TYPE_LIST_SOURCES

HEADERS = VuoList_*.h

INCLUDEPATH += \
	$$ROOT/node \
	$$ROOT/type \
	$$ROOT/runtime

QMAKE_CLEAN += VuoList_*.cc VuoList_*.h *.bc
