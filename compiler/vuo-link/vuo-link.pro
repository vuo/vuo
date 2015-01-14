TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console VuoFramework

include(../../vuo.pri)

SOURCES += \
	vuo-link.cc

LIBS += -rpath @loader_path/.

QMAKE_POST_LINK += cp vuo-link ../../framework
