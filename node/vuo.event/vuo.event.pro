TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.event.areAllHit.2.c \
	vuo.event.becameFalse.c \
	vuo.event.becameTrue.c \
	vuo.event.changed.VuoBoolean.c \
	vuo.event.changed.VuoInteger.c \
	vuo.event.changed.VuoReal.c \
	vuo.event.decreased.integer.c \
	vuo.event.decreased.real.c \
	vuo.event.fireOnDisplayRefresh.c \
	vuo.event.fireOnStart.c \
	vuo.event.increased.integer.c \
	vuo.event.increased.real.c \
	vuo.event.spinOffEvent.c

include(../../module.pri)
