TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

GENERIC_NODE_SOURCES += \
	vuo.vertices.make.points.c \
	vuo.vertices.make.lines.c \
	vuo.vertices.make.lineStrips.c

NODE_SOURCES += \
	vuo.vertices.make.parametric.c \
	vuo.vertices.make.sphere.c \
	vuo.vertices.make.square.c \
	vuo.vertices.make.triangle.c

# OTHER_FILES instead of HEADERS, to avoid including in Vuo.framework
OTHER_FILES += \
	VuoMakeVerticesFromPositions.h

include(../../module.pri)
