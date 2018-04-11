TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.transform.combine.c \
	vuo.transform.get.quaternion.c \
	vuo.transform.get.rotation.2d.c \
	vuo.transform.get.rotation.c \
	vuo.transform.get.rotation.x.c \
	vuo.transform.get.rotation.y.c \
	vuo.transform.get.rotation.z.c \
	vuo.transform.get.scale.2d.c \
	vuo.transform.get.scale.2d.x.c \
	vuo.transform.get.scale.2d.y.c \
	vuo.transform.get.scale.c \
	vuo.transform.get.scale.x.c \
	vuo.transform.get.scale.y.c \
	vuo.transform.get.scale.z.c \
	vuo.transform.get.translation.2d.c \
	vuo.transform.get.translation.2d.x.c \
	vuo.transform.get.translation.2d.y.c \
	vuo.transform.get.translation.c \
	vuo.transform.get.translation.x.c \
	vuo.transform.get.translation.y.c \
	vuo.transform.get.translation.z.c \
	vuo.transform.make.2d.c \
	vuo.transform.make.c \
	vuo.transform.make.list.trs.c \
	vuo.transform.make.quaternion.c

include(../../module.pri)
