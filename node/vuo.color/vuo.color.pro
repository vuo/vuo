TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.color.average.c \
	vuo.color.blend.c \
	vuo.color.dmx.list.c \
	vuo.color.get.hex.c \
	vuo.color.get.hsl.c \
	vuo.color.get.rgb.c \
	vuo.color.list.dmx.c \
	vuo.color.make.hex.c \
	vuo.color.make.hsl.c \
	vuo.color.make.rgb.c

TYPE_SOURCES += \
	VuoDmxColorMap.c

HEADERS += \
	VuoDmxColorMap.h

include(../../module.pri)
