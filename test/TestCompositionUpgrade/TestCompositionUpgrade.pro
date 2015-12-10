TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase qtCore qtGui qtTest graphviz VuoFramework VuoBase VuoCompiler VuoEditor VuoRenderer TestCompositionExecution json
TARGET = TestCompositionUpgrade

include(../../vuo.pri)

QMAKE_RPATHDIR = $$ROOT/framework
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

SOURCES += \
	TestCompositionUpgrade.cc

LIBS += $$ROOT/framework/Vuo.framework/Modules/libVuoHeap.dylib
