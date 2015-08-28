TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase qtTest graphviz VuoFramework VuoCompiler VuoEditor TestCompositionExecution json
TARGET = TestCompositionUpgrade

include(../../vuo.pri)

QMAKE_RPATHDIR = $$ROOT/framework
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

SOURCES += \
	TestCompositionUpgrade.cc
