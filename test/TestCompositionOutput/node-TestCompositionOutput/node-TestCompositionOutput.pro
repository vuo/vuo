TEMPLATE = lib
CONFIG += no_link target_predeps staticlib VuoNoLibrary

include(../../../vuo.pri)

NODE_SOURCES += \
	vuo.test.consoleInput.c \
	vuo.test.delay.c \
	vuo.test.mouse.c \
	vuo.test.wallTime.c

OTHER_FILES += $$NODE_SOURCES
