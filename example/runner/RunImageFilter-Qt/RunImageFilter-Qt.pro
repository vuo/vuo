TEMPLATE = app
CONFIG += qt moc
QT += core gui widgets printsupport opengl
cache()

resources.files = \
	RippleImage.vuo \
	OttoOperatesTheRoller.jpg
resources.path = Contents/Resources
QMAKE_BUNDLE_DATA += resources

HEADERS += \
	GLWidget.hh

SOURCES += \
	main.cc \
	GLWidget.cc

OTHER_FILES += \
	RippleImage.vuo

VUO_FRAMEWORK_PATH=../../../framework

QMAKE_CXX = $$VUO_FRAMEWORK_PATH/Vuo.framework/Frameworks/llvm.framework/Helpers/clang++
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_CXX = $$QMAKE_CXX
QMAKE_LINK = $$QMAKE_CXX
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_LINK = $$QMAKE_LINK
QMAKE_CXXFLAGS += -F$$VUO_FRAMEWORK_PATH
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-private-field
QMAKE_LFLAGS += -F $$VUO_FRAMEWORK_PATH -mmacosx-version-min=10.7 \
	$$VUO_FRAMEWORK_PATH/Vuo.framework/Modules/libVuoHeap.dylib \
	$$VUO_FRAMEWORK_PATH/Vuo.framework/Modules/libVuoGlContext.dylib
QMAKE_RPATHDIR = $$VUO_FRAMEWORK_PATH
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

QMAKE_CLEAN = -r $${TARGET}.app

# Rebuild this example when Vuo.framework's headers change.
DEPENDPATH += $$VUO_FRAMEWORK_PATH/Vuo.framework/Headers

LIBS += -framework Vuo -framework OpenGL

# In an app you plan to distribute, you should copy the Vuo framework to $${TARGET}.app/Contents/Frameworks,
# and change QMAKE_RPATHDIR to $${TARGET}.app/Contents/Frameworks.
