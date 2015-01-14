TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)


NODE_SOURCES += \
	vuo.image.filter.blend.c \
	vuo.image.filter.ripple.c \
	vuo.image.filter.twirl.c \
	vuo.image.get.c \
	vuo.image.render.window.c

OTHER_FILES += $$NODE_SOURCES


TYPE_SOURCES += \
	VuoBlendMode.c

OTHER_FILES += $$TYPE_SOURCES

HEADERS += \
	VuoBlendMode.h


include(../../module.pri)
