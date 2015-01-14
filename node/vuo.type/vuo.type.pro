TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.type.boolean.event.c \
	vuo.type.boolean.integer.c \
	vuo.type.boolean.text.c \
	vuo.type.color.event.c \
	vuo.type.frameRequest.event.c \
	vuo.type.image.event.c \
	vuo.type.integer.boolean.c\
	vuo.type.integer.event.c \
	vuo.type.integer.point2d.x.c \
	vuo.type.integer.point2d.y.c \
	vuo.type.integer.point3d.x.c \
	vuo.type.integer.point3d.y.c \
	vuo.type.integer.point3d.z.c \
	vuo.type.integer.point4d.w.c \
	vuo.type.integer.point4d.x.c \
	vuo.type.integer.point4d.y.c \
	vuo.type.integer.point4d.z.c \
	vuo.type.integer.real.c \
	vuo.type.integer.text.c \
	vuo.type.leapFrame.event.c \
	vuo.type.leapHand.event.c \
	vuo.type.leapPointable.event.c \
	vuo.type.leapPointableType.event.c \
	vuo.type.midiController.event.c \
	vuo.type.midiDevice.event.c \
	vuo.type.midiNode.event.c \
	vuo.type.mouseButtonAction.event.c \
	vuo.type.point2d.event.c \
	vuo.type.point2d.point3d.xy.c \
	vuo.type.point2d.point4d.xy.c \
	vuo.type.point2d.real.x.c \
	vuo.type.point2d.real.y.c \
	vuo.type.point3d.event.c \
	vuo.type.point3d.point2d.xy.c \
	vuo.type.point3d.point4d.xyz.c \
	vuo.type.point3d.real.x.c \
	vuo.type.point4d.event.c \
	vuo.type.point4d.point2d.xy.c \
	vuo.type.point4d.point3d.xyz.c \
	vuo.type.point4d.real.x.c \
	vuo.type.real.event.c \
	vuo.type.real.point2d.x.c \
	vuo.type.real.point2d.y.c \
	vuo.type.real.point3d.x.c \
	vuo.type.real.point3d.y.c \
	vuo.type.real.point3d.z.c \
	vuo.type.real.point4d.w.c \
	vuo.type.real.point4d.x.c \
	vuo.type.real.point4d.y.c \
	vuo.type.real.point4d.z.c \
	vuo.type.real.text.c \
	vuo.type.sceneObject.event.c \
	vuo.type.shader.event.c \
	vuo.type.syphonServerDescription.event.c \
	vuo.type.text.boolean.c \
	vuo.type.text.event.c \
	vuo.type.text.integer.c \
	vuo.type.text.real.c \
	vuo.type.transform.event.c \
	vuo.type.vertices.event.c

OTHER_FILES += $$NODE_SOURCES

NODE_INCLUDEPATH += \
	../vuo.leap \
	../vuo.midi \
	../vuo.syphon

include(../../module.pri)
