TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase graphviz VuoLLVM qtTest VuoBase
TARGET = TestVuoProtocol

include(../../vuo.pri)

SOURCES += \
	TestVuoProtocol.cc
