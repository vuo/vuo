TEMPLATE = aux

include(../../../vuo.pri)

NODE_SOURCES += \
	vuo.test.delay.c \
	vuo.test.firePeriodicallyWithCount.c \
	vuo.test.writeTimeToFile.c

OTHER_FILES += $$NODE_SOURCES

include(../../../module.pri)
