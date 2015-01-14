TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.console.window.c

OTHER_FILES += $$NODE_SOURCES

include(../../module.pri)
