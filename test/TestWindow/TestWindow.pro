TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase qtTest VuoFramework
TARGET = TestWindow

include(../../vuo.pri)

QMAKE_RPATHDIR = $$ROOT/framework
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

LIBS += \
	$$ROOT/library/libmodule.o

SOURCES += \
	TestWindow.cc
