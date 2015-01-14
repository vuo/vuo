TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.hold.VuoBlendMode.c \
	vuo.hold.VuoBoolean.c \
	vuo.hold.VuoColor.c \
	vuo.hold.VuoCountWrapMode.c \
	vuo.hold.VuoCurve.c \
	vuo.hold.VuoCurveDomain.c \
	vuo.hold.VuoFrameRequest.c \
	vuo.hold.VuoGradientNoise.c \
	vuo.hold.VuoImage.c \
	vuo.hold.VuoInteger.c \
	vuo.hold.VuoLayer.c \
	vuo.hold.VuoLeapFrame.c \
	vuo.hold.VuoLeapHand.c \
	vuo.hold.VuoLeapPointable.c \
	vuo.hold.VuoLeapPointableType.c \
	vuo.hold.VuoLoopType.c \
	vuo.hold.VuoMidiController.c \
	vuo.hold.VuoMidiDevice.c \
	vuo.hold.VuoMidiNote.c \
	vuo.hold.VuoMouseButtonAction.c \
	vuo.hold.VuoNoise.c \
	vuo.hold.VuoPoint2d.c \
	vuo.hold.VuoPoint3d.c \
	vuo.hold.VuoPoint4d.c \
	vuo.hold.VuoReal.c \
	vuo.hold.VuoSceneObject.c \
	vuo.hold.VuoShader.c \
	vuo.hold.VuoSyphonServerDescription.c \
	vuo.hold.VuoText.c \
	vuo.hold.VuoTransform.c \
	vuo.hold.VuoTransform2d.c \
	vuo.hold.VuoVertices.c \
	vuo.hold.VuoWave.c

NODE_INCLUDEPATH += \
	../vuo.image \
	../vuo.layer \
	../vuo.leap \
	../vuo.math \
	../vuo.midi \
	../vuo.movie \
	../vuo.noise \
	../vuo.syphon

include(../../module.pri)
