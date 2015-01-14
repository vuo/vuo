TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.vertices.make.points.2d.c \
	vuo.vertices.make.points.3d.c \
	vuo.vertices.make.lines.2d.c \
	vuo.vertices.make.lines.3d.c \
	vuo.vertices.make.lineStrips.2d.c \
	vuo.vertices.make.lineStrips.3d.c \
	vuo.vertices.make.parametric.c \
	vuo.vertices.make.sphere.c \
	vuo.vertices.make.square.c \
	vuo.vertices.make.triangle.c

include(../../module.pri)
