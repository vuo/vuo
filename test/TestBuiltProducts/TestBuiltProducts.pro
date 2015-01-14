TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase VuoLLVM qtTest VuoPCH VuoBase
TARGET = TestBuiltProducts

include(../../vuo.pri)

SOURCES += \
	TestBuiltProducts.cc
