TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)


NODE_SOURCES += \
	vuo.math.add.integer.c \
	vuo.math.add.real.c \
	vuo.math.areEqual.integer.c \
	vuo.math.areEqual.real.c \
	vuo.math.count.integer.c \
	vuo.math.count.real.c \
	vuo.math.countWithinRange.integer.c \
	vuo.math.countWithinRange.real.c \
	vuo.math.divide.integer.c \
	vuo.math.divide.real.c \
	vuo.math.isGreaterThan.integer.c \
	vuo.math.isGreaterThan.real.c \
	vuo.math.isLessThan.integer.c \
	vuo.math.isLessThan.real.c \
	vuo.math.limitToRange.integer.c \
	vuo.math.limitToRange.real.c \
	vuo.math.max.integer.c \
	vuo.math.max.real.c \
	vuo.math.min.integer.c \
	vuo.math.min.real.c \
	vuo.math.multiply.integer.c \
	vuo.math.multiply.real.c \
	vuo.math.round.c \
	vuo.math.roundDown.c \
	vuo.math.roundUp.c \
	vuo.math.scale.c \
	vuo.math.subtract.integer.c \
	vuo.math.subtract.real.c \
	vuo.math.wave.c

OTHER_FILES += $$NODE_SOURCES


TYPE_SOURCES += \
	VuoCountWrapMode.c \
	VuoCurve.c \
	VuoCurveDomain.c \
	VuoWave.c

OTHER_FILES += $$TYPE_SOURCES

HEADERS += \
	VuoCountWrapMode.h \
	VuoCurve.h \
	VuoCurveDomain.h \
	VuoWave.h


include(../../module.pri)
