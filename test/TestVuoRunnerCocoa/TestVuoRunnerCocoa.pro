TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase qtTest graphviz VuoFramework json
TARGET = TestVuoRunnerCocoa

include(../../vuo.pri)
include(../test.pri)

QMAKE_RPATHDIR = $$ROOT/framework
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

QMAKE_OBJECTIVE_CFLAGS += -F$$ROOT/framework

SOURCES += \
	TestVuoRunnerCocoa.mm
