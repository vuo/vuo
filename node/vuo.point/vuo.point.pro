TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.point.add.2d.c \
	vuo.point.add.3d.c \
	vuo.point.add.4d.c \
	vuo.point.distance.2d.c \
	vuo.point.distance.3d.c \
	vuo.point.distance.4d.c \
	vuo.point.get.2d.c \
	vuo.point.get.3d.c \
	vuo.point.get.4d.c \
	vuo.point.make.2d.c \
	vuo.point.make.3d.c \
	vuo.point.make.4d.c \
	vuo.point.multiply.quaternion.c \
	vuo.point.multiply.scalar.2d.c \
	vuo.point.multiply.scalar.3d.c \
	vuo.point.multiply.scalar.4d.c \
	vuo.point.normalize.2d.c \
	vuo.point.normalize.3d.c \
	vuo.point.normalize.4d.c \
	vuo.point.sort.distance.2d.c \
	vuo.point.sort.distance.3d.c \
	vuo.point.sort.distance.4d.c \
	vuo.point.sort.distance.w.4d.c \
	vuo.point.sort.distance.x.2d.c \
	vuo.point.sort.distance.x.3d.c \
	vuo.point.sort.distance.x.4d.c \
	vuo.point.sort.distance.y.2d.c \
	vuo.point.sort.distance.y.3d.c \
	vuo.point.sort.distance.y.4d.c \
	vuo.point.sort.distance.z.3d.c \
	vuo.point.sort.distance.z.4d.c \
	vuo.point.subtract.2d.c \
	vuo.point.subtract.3d.c \
	vuo.point.subtract.4d.c

include(../../module.pri)
