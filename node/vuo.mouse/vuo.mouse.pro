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

NODE_LIBRARY_SOURCES += \
	VuoMouse.m

# OTHER_FILES instead of HEADERS, to avoid including in Vuo.framework
OTHER_FILES += \
	VuoMouseButton.h \
	VuoMouse.h

include(../../module.pri)
