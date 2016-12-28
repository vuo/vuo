TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

GENERIC_NODE_SOURCES += \
	vuo.mesh.make.points.c \
	vuo.mesh.make.lines.c \
	vuo.mesh.make.lineStrips.c

NODE_SOURCES += \
	vuo.mesh.make.parametric.c \
	vuo.mesh.make.sphere.c \
	vuo.mesh.make.square.c \
	vuo.mesh.make.triangle.c \
	vuo.mesh.populated.c

include(../../module.pri)
