TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.image.filter.blend.c \
#	vuo.image.filter.blur.c \
	vuo.image.filter.ripple.c \
	vuo.image.filter.twirl.c \
	vuo.image.get.c \
#	vuo.image.get.size.c \
	vuo.image.make.color.c \
	vuo.image.make.text.c \
	vuo.image.render.window.c

TYPE_SOURCES += \
	VuoBlendMode.c

HEADERS += \
	VuoBlendMode.h

NODE_INCLUDEPATH += \
	../vuo.font

include(../../module.pri)
