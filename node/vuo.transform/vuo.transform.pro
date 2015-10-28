TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.transform.combine.c \
	vuo.transform.get.quaternion.c \
	vuo.transform.get.rotation.c \
	vuo.transform.get.rotation.2d.c \
	vuo.transform.get.scale.c \
	vuo.transform.get.scale.2d.c \
	vuo.transform.get.translation.c \
	vuo.transform.get.translation.2d.c \
	vuo.transform.make.c \
	vuo.transform.make.2d.c \
	vuo.transform.make.quaternion.c

include(../../module.pri)
