TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.time.firePeriodically.c \
	vuo.time.measureTime.c

include(../../module.pri)
