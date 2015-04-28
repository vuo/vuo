TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.layer.align.window.c \
	vuo.layer.make.c \
	vuo.layer.copy.c \
	vuo.layer.copy.trs.c \
	vuo.layer.make.color.c \
	vuo.layer.make.realSize.c \
	vuo.layer.make.realSize.shadow.c \
	vuo.layer.make.shadow.c \
	vuo.layer.make.gradient.linear.c \
	vuo.layer.make.gradient.radial.c \
	vuo.layer.combine.c \
	vuo.layer.drag.c \
	vuo.layer.render.window.c \
	vuo.layer.render.image.c \
	vuo.layer.within.c

TYPE_SOURCES += \
	VuoLayer.c \
	VuoRenderedLayers.c

HEADERS += \
	VuoLayer.h \
	VuoRenderedLayers.h

NODE_INCLUDEPATH += \
	../vuo.mouse

include(../../module.pri)
