TEMPLATE = aux

include(../../../vuo.pri)

NODE_SOURCES += \
	vuo.test.consoleInput.c \
	vuo.test.delay.c \
	vuo.test.mouse.c \
	vuo.test.wallTime.c

OTHER_FILES += $$NODE_SOURCES

include(../../../module.pri)
