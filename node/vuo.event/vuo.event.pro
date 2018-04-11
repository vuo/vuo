TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

GENERIC_NODE_SOURCES += \
	vuo.event.allowChanges.c \
	vuo.event.allowChanges2.c \
	vuo.event.allowFirstValue.c \
	vuo.event.changed.c \
	vuo.event.changed2.c \
	vuo.event.decreased.c \
	vuo.event.decreased2.c \
	vuo.event.emptyList.c \
	vuo.event.increased.c \
	vuo.event.increased2.c \
	vuo.event.spinOffValue.c

NODE_SOURCES += \
	vuo.event.areAllHit.2.c \
	vuo.event.becameFalse.c \
	vuo.event.becameFalse2.c \
	vuo.event.allowFirst.c \
	vuo.event.becameTrue.c \
	vuo.event.fireOnDisplayRefresh.c \
	vuo.event.fireOnStart.c \
	vuo.event.spinOffEvent.c \
	vuo.event.spinOffEvents.c

NODE_INCLUDEPATH += \
	../vuo.artnet

include(../../module.pri)
