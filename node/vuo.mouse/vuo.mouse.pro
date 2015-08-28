TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.mouse.button.c \
	vuo.mouse.click.c \
	vuo.mouse.delta.c \
	vuo.mouse.drag.c \
	vuo.mouse.move.c \
	vuo.mouse.scroll.c \
	vuo.mouse.status.c

TYPE_SOURCES += \
	VuoMouseButton.c

HEADERS += \
	VuoMouseButton.h

NODE_LIBRARY_SOURCES += \
	VuoMouse.m

HEADERS += \
	VuoMouse.h

include(../../module.pri)
