TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase graphviz VuoLLVM qtCore qtGui qtTest VuoBase VuoRuntime VuoCompiler VuoRenderer
TARGET = TestVuoRenderer
QT += testlib

include(../../vuo.pri)

SOURCES += TestVuoRenderer.cc
