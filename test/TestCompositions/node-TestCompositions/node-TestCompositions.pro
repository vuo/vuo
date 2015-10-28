TEMPLATE = aux

include(../../../vuo.pri)

NODE_SOURCES += \
	vuo.test.delay.c \
	vuo.test.wallTime.c

include(../../../module.pri)
