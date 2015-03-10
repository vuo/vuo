TEMPLATE = app
CONFIG -= qt
cache()

QMAKE_INFO_PLIST = Info.plist

resources.files = \
	MainMenu.nib \
	RippleImage.vuo \
	OttoOperatesTheRoller.jpg
resources.path = Contents/Resources
QMAKE_BUNDLE_DATA += resources

OBJECTIVE_SOURCES += \
	main.m \
	RunImageFilterView.mm

OTHER_FILES += \
	RippleImage.vuo

VUO_FRAMEWORK_PATH=../../../framework

QMAKE_CC = $$VUO_FRAMEWORK_PATH/Vuo.framework/MacOS/Clang/bin/clang++
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_CC = $$QMAKE_CC
QMAKE_LINK = $$QMAKE_CC
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_LINK = $$QMAKE_LINK
QMAKE_OBJECTIVE_CFLAGS += -F $$VUO_FRAMEWORK_PATH
QMAKE_OBJECTIVE_CFLAGS_WARN_ON += -Wno-unused-parameter
QMAKE_LFLAGS += -F $$VUO_FRAMEWORK_PATH -mmacosx-version-min=10.6 \
	$$VUO_FRAMEWORK_PATH/Vuo.framework/Frameworks/VuoRuntime.framework/libVuoHeap.dylib \
	$$VUO_FRAMEWORK_PATH/Vuo.framework/Modules/libVuoGlContext.dylib
QMAKE_RPATHDIR = $$VUO_FRAMEWORK_PATH
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

QMAKE_CLEAN = -r $${TARGET}.app

# Rebuild this example when Vuo.framework's headers change.
DEPENDPATH += $$VUO_FRAMEWORK_PATH/Vuo.framework/Headers

LIBS += -framework Vuo -framework Cocoa -framework OpenGL -framework CoreVideo

# In an app you plan to distribute, you should copy the Vuo framework to $${TARGET}.app/Contents/Frameworks,
# and change QMAKE_RPATHDIR to $${TARGET}.app/Contents/Frameworks.
