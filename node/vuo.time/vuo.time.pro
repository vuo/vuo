TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.time.after.timeOfDay.c \
	vuo.time.before.timeOfDay.c \
	vuo.time.bpm.seconds.c \
	vuo.time.elapsed.c \
	vuo.time.equal.dateTime.c \
	vuo.time.equal.timeOfDay.c \
	vuo.time.fetch.c \
	vuo.time.firePeriodically.c \
	vuo.time.format.c \
	vuo.time.get.c \
	vuo.time.make.c \
	vuo.time.measureTime.c \
	vuo.time.offset.c \
	vuo.time.relative.get.c \
	vuo.time.relative.make.c \
	vuo.time.round.c \
	vuo.time.schedule.c \
	vuo.time.seconds.bpm.c

TYPE_SOURCES += \
	VuoDurationType.c \
	VuoRelativeTime.c \
	VuoTime.c \
	VuoTimeFormat.c \
	VuoTimeUnit.c \
	VuoWeekday.c

HEADERS += \
	VuoDurationType.h \
	VuoRelativeTime.h \
	VuoTime.h \
	VuoTimeFormat.h \
	VuoTimeUnit.h \
	VuoWeekday.h

TYPE_INCLUDEPATH = \
	$$ROOT/node/vuo.math

NODE_INCLUDEPATH = \
	$$ROOT/node/vuo.math

include(../../module.pri)
