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
	vuo.select.out.boolean.c \
	vuo.select.in.list.2.c \
	vuo.select.in.list.8.c \
	vuo.select.in.list.boolean.c \
	vuo.select.latest.list.2.c \
	vuo.select.latest.list.8.c \
	vuo.select.out.list.2.c \
	vuo.select.out.list.8.c \
	vuo.select.out.list.boolean.c

include(../../module.pri)
