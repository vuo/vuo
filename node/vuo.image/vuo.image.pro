TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.image.crop.c \
	vuo.image.crop.pixels.c \
	vuo.image.blend.c \
	vuo.image.blur.c \
	vuo.image.color.adjust.c \
	vuo.image.color.map.c \
	vuo.image.color.mask.brightness.c \
	vuo.image.pixellate.c \
	vuo.image.color.invert.c \
	vuo.image.ripple.c \
	vuo.image.twirl.c \
	vuo.image.vignette.c \
	vuo.image.flip.horizontal.c \
	vuo.image.flip.vertical.c \
	vuo.image.get.c \
	vuo.image.get.size.c \
	vuo.image.make.checkerboard.c \
	vuo.image.make.color.c \
	vuo.image.make.gradient.linear.c \
	vuo.image.make.gradient.radial.c \
	vuo.image.make.text.c \
#	vuo.image.make.triangle.c \
	vuo.image.apply.mask.c \
	vuo.image.resize.c \
	vuo.image.resize.larger.c \
	vuo.image.render.window.c \
#	vuo.image.rotate.c \
	vuo.image.set.wrapMode.c

NODE_INCLUDEPATH += \
	../vuo.audio \
	../vuo.font

TYPE_SOURCES += \
	VuoThresholdType.c

HEADERS += \
	VuoThresholdType.h

include(../../module.pri)
