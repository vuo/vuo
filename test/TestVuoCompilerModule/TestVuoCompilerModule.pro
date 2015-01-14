TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase graphviz VuoLLVM qtTest VuoBase VuoRuntime VuoCompiler TestVuoCompiler
TARGET = TestVuoCompilerModule

include(../../vuo.pri)

SOURCES += \
	TestVuoCompilerModule.cc
