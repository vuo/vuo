TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorMathExpressionList

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorMathExpressionList.cc

HEADERS += \
	VuoInputEditorMathExpressionList.hh

OTHER_FILES += \
	VuoInputEditorMathExpressionList.json

INCLUDEPATH += \
	$$ROOT/library

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoMathExpressionList.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/VuoDictionary_VuoText_VuoReal.o \
	$$ROOT/type/list/VuoList_VuoInteger.o \
	$$ROOT/type/list/VuoList_VuoReal.o \
	$$ROOT/type/list/VuoList_VuoText.o \
	$$ROOT/node/vuo.noise/VuoGradientNoiseCommon.bc \
	$$ROOT/library/VuoMathExpressionParser.o \
	$$MUPARSER_ROOT/lib/libmuparser.a
