TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.event.fireOnDisplayRefresh.c \
	vuo.event.fireOnStart.c \
	vuo.event.spinOffEvent.c

OTHER_FILES += $$NODE_SOURCES

include(../../module.pri)
