TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase qtTest VuoFramework TestCompositionExecution json
TARGET = TestCompositionOutput

include(../../vuo.pri)

QMAKE_RPATHDIR = $$ROOT/framework
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

SOURCES += \
	TestCompositionOutput.cc \
	PortConfiguration.cc

HEADERS += \
	PortConfiguration.hh
