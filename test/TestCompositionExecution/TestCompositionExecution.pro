TEMPLATE = lib
CONFIG += staticlib qtTest VuoFramework
TARGET = TestCompositionExecution

include(../../vuo.pri)

QMAKE_RPATHDIR = $$ROOT/framework
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

HEADERS += \
	TestCompositionExecution.hh

SOURCES += \
	TestCompositionExecution.cc
