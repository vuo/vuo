TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

GENERIC_NODE_SOURCES += \
	vuo.point.add.c \
	vuo.point.distance.c \
	vuo.point.make.curve.c \
	vuo.point.multiply.scalar.c \
	vuo.point.normalize.c \
	vuo.point.sort.distance.c \
	vuo.point.sort.distance.x.c \
	vuo.point.sort.distance.y.c \
	vuo.point.sort.distance.z.c \
	vuo.point.subtract.c

NODE_SOURCES += \
	vuo.point.get.VuoPoint2d.c \
	vuo.point.get.VuoPoint3d.c \
	vuo.point.get.VuoPoint4d.c \
	vuo.point.make.parametric.c \
	vuo.point.make.parametric.grid.c \
	vuo.point.make.VuoPoint2d.c \
	vuo.point.make.VuoPoint3d.c \
	vuo.point.make.VuoPoint4d.c \
	vuo.point.sort.distance.w.VuoPoint4d.c \
	vuo.point.within.box.c \
	vuo.point.within.circle.c \
	vuo.point.within.rectangle.c \
	vuo.point.within.sphere.c

NODE_INCLUDEPATH += \
	../vuo.motion

include(../../module.pri)
