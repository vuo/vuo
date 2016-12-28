TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase qtTest VuoFramework TestCompositionExecution
TARGET = TestEventDropping

include(../../vuo.pri)

QMAKE_RPATHDIR = $$ROOT/framework
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

SOURCES += \
	TestEventDropping.cc
