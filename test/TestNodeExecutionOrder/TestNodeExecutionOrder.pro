TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase qtTest VuoFramework TestCompositionExecution
TARGET = TestNodeExecutionOrder

include(../../vuo.pri)

QMAKE_RPATHDIR = $$ROOT/framework
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

SOURCES += \
	TestNodeExecutionOrder.cc
