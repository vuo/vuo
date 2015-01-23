TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.select.in.2.event.c \
	vuo.select.in.event.c \
	vuo.select.out.2.event.c \
	vuo.select.out.event.c

GENERIC_NODE_SOURCES += \
	vuo.select.in.2.c \
	vuo.select.in.c \
	vuo.select.latest.2.c \
	vuo.select.out.2.c \
	vuo.select.out.c

include(../../module.pri)
