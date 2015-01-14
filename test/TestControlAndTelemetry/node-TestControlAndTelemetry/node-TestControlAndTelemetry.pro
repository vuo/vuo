TEMPLATE = aux

include(../../../vuo.pri)

NODE_SOURCES += \
	vuo.test.delay.c \
	vuo.test.firePeriodicallyWithCount.c \
	vuo.test.spinOffWithCount.c \
	vuo.test.writeTimeToFile.c

include(../../../module.pri)
