TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.color.get.hsl.c \
	vuo.color.get.rgb.c \
	vuo.color.make.hsl.c \
	vuo.color.make.rgb.c

OTHER_FILES += $$NODE_SOURCES

include(../../module.pri)
