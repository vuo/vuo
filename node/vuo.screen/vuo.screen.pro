TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.screen.capture.c \
	vuo.screen.get.c \
	vuo.screen.list.c \
	vuo.screen.make.name.c

include(../../module.pri)
