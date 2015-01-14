TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.transform.make.c \
	vuo.transform.make.2d.c \
	vuo.transform.make.quaternion.c

include(../../module.pri)
