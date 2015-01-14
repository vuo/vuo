TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase graphviz VuoLLVM qtTest VuoBase VuoRuntime VuoCompiler TestVuoCompiler
TARGET = TestVuoCompilerNode

include(../../vuo.pri)

SOURCES += \
	TestVuoCompilerNode.cc
