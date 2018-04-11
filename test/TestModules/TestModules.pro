TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase qtTest VuoFramework
TARGET = TestModules

include(../../vuo.pri)
include(../test.pri)

QMAKE_RPATHDIR = $$ROOT/framework

SOURCES += \
	TestModules.cc

INCLUDEPATH += \
	$$WJELEMENT_ROOT/include

LIBS += \
	$$WJELEMENT_ROOT/lib/libwjelement.dylib \
	$$WJELEMENT_ROOT/lib/libwjreader.dylib
