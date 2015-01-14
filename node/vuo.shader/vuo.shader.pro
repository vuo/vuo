TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.shader.make.color.c \
	vuo.shader.make.image.c \
	vuo.shader.make.normal.c

OTHER_FILES += $$NODE_SOURCES

include(../../module.pri)
