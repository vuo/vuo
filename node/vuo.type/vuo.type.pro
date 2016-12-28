TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.type.audioframe.audiosamples.c \
	vuo.type.audioframe.real.c \
	vuo.type.boolean.integer.c \
	vuo.type.boolean.real.c \
	vuo.type.boolean.text.c \
	vuo.type.data.text.c \
	vuo.type.integer.boolean.c\
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
	vuo.type.list.audio.real.c \
	vuo.type.list.integer.real.c \
	vuo.type.list.point2d.point3d.xy.c \
	vuo.type.list.point2d.real.x.c \
	vuo.type.list.point2d.real.y.c \
	vuo.type.list.point3d.point2d.xy.c \
	vuo.type.list.point3d.real.x.c \
	vuo.type.list.point3d.real.y.c \
	vuo.type.list.point3d.real.z.c \
	vuo.type.list.real.integer.c \
	vuo.type.list.real.point2d.x.c \
	vuo.type.list.real.point2d.y.c \
	vuo.type.list.real.point3d.x.c \
	vuo.type.list.real.point3d.y.c \
	vuo.type.list.real.point3d.z.c \
	vuo.type.point2d.point3d.xy.c \
	vuo.type.point2d.point4d.xy.c \
	vuo.type.point2d.real.x.c \
	vuo.type.point2d.real.y.c \
	vuo.type.point3d.point2d.xy.c \
	vuo.type.point3d.point4d.xyz.c \
	vuo.type.point3d.real.x.c \
	vuo.type.point4d.point2d.xy.c \
	vuo.type.point4d.point3d.xyz.c \
	vuo.type.point4d.real.x.c \
	vuo.type.real.boolean.c \
	vuo.type.real.point2d.x.c \
	vuo.type.real.point2d.xy.c \
	vuo.type.real.point2d.y.c \
	vuo.type.real.point3d.x.c \
	vuo.type.real.point3d.xyz.c \
	vuo.type.real.point3d.y.c \
	vuo.type.real.point3d.z.c \
	vuo.type.real.point4d.w.c \
	vuo.type.real.point4d.x.c \
	vuo.type.real.point4d.y.c \
	vuo.type.real.point4d.z.c \
	vuo.type.real.text.c \
	vuo.type.rotate.point3d.transform.c \
	vuo.type.rotate.real.transform2d.c \
	vuo.type.scale.point2d.transform2d.c \
	vuo.type.scale.point3d.transform.c \
	vuo.type.text.boolean.c \
	vuo.type.text.data.c \
	vuo.type.text.integer.c \
	vuo.type.text.real.c \
	vuo.type.translate.point2d.transform2d.c \
	vuo.type.translate.point3d.transform.c \
	vuo.type.videoframe.image.c \
	vuo.type.videoframe.real.c


NODE_INCLUDEPATH += \
	../vuo.data \
	../vuo.leap \
	../vuo.midi \
	../vuo.video \
	../vuo.syphon

include(../../module.pri)
