TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase graphviz VuoLLVM qtCore qtTest VuoBase
TARGET = TestVuoUtilities

include(../../vuo.pri)

SOURCES += \
	TestVuoUtilities.cc
