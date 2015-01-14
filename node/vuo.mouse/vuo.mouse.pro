TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.mouse.c \
	vuo.mouse.filter.action.c \
	vuo.mouse.get.action.c

OTHER_FILES += $$NODE_SOURCES

include(../../module.pri)
