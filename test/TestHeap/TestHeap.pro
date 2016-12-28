TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase VuoLLVM qtTest VuoPCH VuoBase
TARGET = TestHeap

include(../../vuo.pri)
include(../test.pri)

SOURCES += \
	TestHeap.cc
