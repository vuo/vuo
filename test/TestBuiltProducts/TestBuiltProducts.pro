TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase qtCore qtGui qtTest VuoFramework VuoBase VuoRenderer TestCompositionExecution
TARGET = TestBuiltProducts

include(../../vuo.pri)

QMAKE_RPATHDIR = $$ROOT/framework
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

SOURCES += \
	TestBuiltProducts.cc
