TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.layer.make.c \
	vuo.layer.make.color.c \
	vuo.layer.make.realSize.c \
	vuo.layer.make.gradient.linear.c \
	vuo.layer.make.gradient.radial.c \
	vuo.layer.combine.c \
	vuo.layer.render.window.c \
	vuo.layer.render.image.c

TYPE_SOURCES += \
	VuoLayer.c

HEADERS += \
	VuoLayer.h

include(../../module.pri)
