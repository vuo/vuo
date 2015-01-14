TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase graphviz VuoLLVM qtTest VuoBase VuoRuntime VuoCompiler TestVuoCompiler
TARGET = TestVuoCompilerNodeClass

include(../../vuo.pri)

SOURCES += \
	TestVuoCompilerNodeClass.cc
