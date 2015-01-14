TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console VuoFramework qtGuiIncludes VuoBase VuoRenderer

include(../../vuo.pri)

SOURCES += vuo-export.cc

LIBS += \
	-rpath @loader_path/. \
	-rpath @loader_path/resources \
	-F../../framework/resources \
	-lobjc \
	-framework QtCore \
	-framework QtGui \
	-framework QtWidgets \
	-framework QtPrintSupport

QMAKE_POST_LINK += cp vuo-export ../../framework
