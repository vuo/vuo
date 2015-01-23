TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

GENERIC_NODE_SOURCES += \
	vuo.list.count.c \
	vuo.list.get.c \
	vuo.list.cycle.c

include(../../module.pri)
