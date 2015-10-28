TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

GENERIC_NODE_SOURCES += \
	vuo.data.hold.c \
	vuo.data.hold.list.c \
	vuo.data.share.c \
	vuo.data.share.list.c \
	vuo.data.summarize.c

include(../../module.pri)
