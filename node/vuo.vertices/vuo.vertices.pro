TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.vertices.make.parametric.c \
	vuo.vertices.make.sphere.c \
	vuo.vertices.make.square.c \
	vuo.vertices.make.triangle.c

OTHER_FILES += $$NODE_SOURCES

include(../../module.pri)
