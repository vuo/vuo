TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.window.get.dimensions.c \
	vuo.window.aspectRatio.c \
	vuo.window.aspectRatio.reset.c \
	vuo.window.fullscreen.c \
	vuo.window.position.c \
	vuo.window.resizable.c \
	vuo.window.size.c \
	vuo.window.title.c

include(../../module.pri)
