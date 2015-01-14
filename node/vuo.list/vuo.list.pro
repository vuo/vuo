TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.list.count.VuoLeapHand.c \
	vuo.list.count.VuoLeapPointable.c \
	vuo.list.count.VuoSyphonServerDescription.c \
	vuo.list.get.VuoLeapFrame.c \
	vuo.list.get.VuoLeapHand.c \
	vuo.list.get.VuoLeapPointable.c \
	vuo.list.get.VuoLeapPointableType.c \
	vuo.list.get.VuoBlendMode.c \
	vuo.list.get.VuoBoolean.c \
	vuo.list.get.VuoColor.c \
	vuo.list.get.VuoCountWrapMode.c \
	vuo.list.get.VuoCurve.c \
	vuo.list.get.VuoCurveDomain.c \
	vuo.list.get.VuoFrameRequest.c \
	vuo.list.get.VuoGradientNoise.c \
	vuo.list.get.VuoImage.c \
	vuo.list.get.VuoInteger.c \
	vuo.list.get.VuoMidiController.c \
	vuo.list.get.VuoMidiDevice.c \
	vuo.list.get.VuoMidiNote.c \
	vuo.list.get.VuoMouseButtonAction.c \
	vuo.list.get.VuoNoise.c \
	vuo.list.get.VuoPoint2d.c \
	vuo.list.get.VuoPoint3d.c \
	vuo.list.get.VuoPoint4d.c \
	vuo.list.get.VuoReal.c \
	vuo.list.get.VuoSceneObject.c \
	vuo.list.get.VuoShader.c \
	vuo.list.get.VuoSyphonServerDescription.c \
	vuo.list.get.VuoText.c \
	vuo.list.get.VuoTransform.c \
	vuo.list.get.VuoVertices.c

OTHER_FILES += $$NODE_SOURCES

NODE_INCLUDEPATH += \
	../vuo.image \
	../vuo.leap \
	../vuo.math \
	../vuo.midi \
	../vuo.noise \
	../vuo.syphon

include(../../module.pri)
