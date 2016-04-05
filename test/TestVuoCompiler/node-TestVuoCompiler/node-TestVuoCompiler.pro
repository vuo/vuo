TEMPLATE = aux

include(../../../vuo.pri)

NODE_SOURCES += \
	vuo.test.compatibleWith107.c \
	vuo.test.compatibleWith107And108.c \
	vuo.test.compatibleWith107AndUp.c \
	vuo.test.details.c \
	vuo.test.inputDataPortOrder.c \
	vuo.test.keywords.c \
	vuo.test.multiDigitGenericTypes.c \
	vuo.test.triggerCarryingInteger.c \
	vuo.test.triggerWithOutput.c \
	vuo.test.unicodeDefaultString.c

include(../../../module.pri)
