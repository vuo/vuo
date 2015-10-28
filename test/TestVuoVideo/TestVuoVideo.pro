TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase VuoLLVM qtTest VuoPCH VuoBase
TARGET = TestVuoVideo

include(../../vuo.pri)
include(../test.pri)

SOURCES += \
	TestVuoVideo.cc
