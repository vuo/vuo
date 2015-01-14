TEMPLATE = lib
CONFIG += no_link target_predeps staticlib VuoNoLibrary

include(../../../vuo.pri)

NODE_SOURCES += \
	vuo.test.delay.c \
	vuo.test.firePeriodicallyWithCount.c \
	vuo.test.writeTimeToFile.c

OTHER_FILES += $$NODE_SOURCES
