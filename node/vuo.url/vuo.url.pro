TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.url.get.c \
	vuo.url.get.file.c

include(../../module.pri)
