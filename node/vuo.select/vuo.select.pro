TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.select.in.event.2.c \
	vuo.select.in.event.8.c \
	vuo.select.in.boolean.event.c \
	vuo.select.out.event.2.c \
	vuo.select.out.event.8.c \
	vuo.select.out.boolean.event.c

GENERIC_NODE_SOURCES += \
	vuo.select.in.2.c \
	vuo.select.in.8.c \
	vuo.select.in.boolean.c \
	vuo.select.latest.2.c \
	vuo.select.latest.8.c \
	vuo.select.out.2.c \
	vuo.select.out.8.c \
	vuo.select.out.boolean.c

include(../../module.pri)
