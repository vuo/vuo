TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase VuoLLVM qtTest VuoPCH VuoBase
TARGET = TestVuoMovie

include(../../vuo.pri)
include(../test.pri)

SOURCES += \
	TestVuoMovie.cc
