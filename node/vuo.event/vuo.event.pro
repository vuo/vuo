TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

GENERIC_NODE_SOURCES += \
	vuo.event.changed.c \
	vuo.event.decreased.c \
	vuo.event.emptyList.c \
	vuo.event.increased.c

NODE_SOURCES += \
	vuo.event.areAllHit.2.c \
	vuo.event.becameFalse.c \
	vuo.event.becameTrue.c \
	vuo.event.fireOnDisplayRefresh.c \
	vuo.event.fireOnStart.c \
	vuo.event.spinOffEvent.c

include(../../module.pri)
