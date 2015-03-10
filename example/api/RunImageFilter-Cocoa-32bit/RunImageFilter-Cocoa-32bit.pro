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


FLAGS32 = -m32
QMAKE_CFLAGS += $$FLAGS32
QMAKE_CXXFLAGS += $$FLAGS32
QMAKE_LFLAGS += $$FLAGS32
QMAKE_OBJECTIVE_CFLAGS += $$FLAGS32
QMAKE_OBJECTIVE_CXXFLAGS += $$FLAGS32

FLAGS = $$FLAGS32 -fvisibility-inlines-hidden
QMAKE_CFLAGS_RELEASE += $$FLAGS
QMAKE_CFLAGS_DEBUG += $$FLAGS
QMAKE_CXXFLAGS_RELEASE += $$FLAGS
QMAKE_CXXFLAGS_DEBUG += $$FLAGS


QMAKE_CC = $$VUO_FRAMEWORK_PATH/Vuo.framework/MacOS/Clang/bin/clang++
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_CC = $$QMAKE_CC
QMAKE_LINK = $$QMAKE_CC
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_LINK = $$QMAKE_LINK
QMAKE_OBJECTIVE_CFLAGS += -F $$VUO_FRAMEWORK_PATH
QMAKE_OBJECTIVE_CFLAGS_WARN_ON += -Wno-unused-parameter
QMAKE_LFLAGS += -F $$VUO_FRAMEWORK_PATH -mmacosx-version-min=10.6
QMAKE_RPATHDIR = $$VUO_FRAMEWORK_PATH
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

QMAKE_CLEAN = -r $${TARGET}.app

# Rebuild this example when Vuo.framework's headers change.
DEPENDPATH += $$VUO_FRAMEWORK_PATH/Vuo.framework/Headers

LIBS += -framework Vuo -framework Cocoa -framework OpenGL -framework CoreVideo


# In an app you plan to distribute, you should copy the Vuo framework to $${TARGET}.app/Contents/Frameworks,
# and change QMAKE_RPATHDIR to $${TARGET}.app/Contents/Frameworks.
