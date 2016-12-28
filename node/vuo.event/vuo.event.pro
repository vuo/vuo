TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

GENERIC_NODE_SOURCES += \
	vuo.event.allowChanges.c \
	vuo.event.changed.c \
	vuo.event.decreased.c \
	vuo.event.emptyList.c \
	vuo.event.increased.c

NODE_SOURCES += \
	vuo.event.areAllHit.2.c \
	vuo.event.becameFalse.c \
	vuo.event.allowFirst.c \
	vuo.event.becameTrue.c \
	vuo.event.fireOnDisplayRefresh.c \
	vuo.event.fireOnStart.c \
	vuo.event.spinOffEvent.c \
	vuo.event.spinOffEvents.c

NODE_INCLUDEPATH += \
	../vuo.artnet

include(../../module.pri)
