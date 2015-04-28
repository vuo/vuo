TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

GENERIC_NODE_SOURCES += \
	vuo.math.absolute.c \
	vuo.math.add.c \
	vuo.math.areEqual.c \
	vuo.math.cos.c \
	vuo.math.count.c \
	vuo.math.isGreaterThan.c \
	vuo.math.isLessThan.c \
	vuo.math.isWithinRange.c \
	vuo.math.max.c \
	vuo.math.min.c \
	vuo.math.multiply.c \
	vuo.math.sin.c \
	vuo.math.snap.c \
	vuo.math.subtract.c \
	vuo.math.tan.c

NODE_SOURCES += \
	vuo.math.countWithinRange.VuoInteger.c \
	vuo.math.countWithinRange.VuoReal.c \
	vuo.math.divide.VuoInteger.c \
	vuo.math.divide.VuoReal.c \
	vuo.math.limitToRange.VuoInteger.c \
	vuo.math.limitToRange.VuoReal.c \
	vuo.math.round.c \
	vuo.math.roundDown.c \
	vuo.math.roundUp.c \
	vuo.math.scale.c

include(../../module.pri)
