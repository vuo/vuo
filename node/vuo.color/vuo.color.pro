TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.color.average.c \
	vuo.color.blend.c \
	vuo.color.get.hsl.c \
	vuo.color.get.rgb.c \
	vuo.color.make.hsl.c \
	vuo.color.make.rgb.c

include(../../module.pri)
