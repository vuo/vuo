TEMPLATE = lib
CONFIG += no_link target_predeps staticlib VuoNoLibrary

include(../../vuo.pri)

EXAMPLE_SOURCES += \
	BlendImages.vuo \
	CalculateTip.vuo \
	CheckSMSLength.vuo \
	Count.vuo \
	CountAndHold.vuo \
	CountAndOffset.vuo \
	CountDown.vuo \
	CountLeapObjects.vuo \
	CountSometimes.vuo \
	CountWithFeedback.vuo \
	CountWithPublishedPorts.vuo \
	DisplayImage.vuo \
	DisplayLeapHand.vuo \
	DisplayScene.vuo \
	DisplaySquare.vuo \
	ExploreColorSchemes.vuo \
	FlipCoin.vuo \
	HelloWorld.vuo \
	LoadImageAsynchronously.vuo \
	PlayFingerPuppetsWithLeap.vuo \
	PlayTennis.vuo \
	PlayTennisWithLeap.vuo \
	RevealWord.vuo \
	RippleImageOfSphere.vuo \
	SendMIDINotes.vuo \
	ShowMouseClicks.vuo \
	SpinSphere.vuo \
	TwirlImageWithLeap.vuo \
	WalkCaterpillar.vuo \
	WanderImage.vuo \
	WaveSphere.vuo
OTHER_FILES += $$EXAMPLE_SOURCES

VUOLINK_FLAGS = \
	--library-search-path $${ICU_ROOT}/lib \
	--library-search-path $${JSONC_ROOT}/lib \
	--library-search-path $${ZMQ_ROOT}/lib \
	--framework-search-path $${QT_ROOT}/lib
example.input = EXAMPLE_SOURCES
example.output = ${QMAKE_FILE_IN_BASE}
example.depends = $$VUOCOMPILE $$VUOLINK $$ROOT/node/*.bc $$ROOT/type/*.bc $$ROOT/type/list/*.bc
example.commands += \
	$$VUOCOMPILE --output ${QMAKE_FILE_IN_BASE}.bc ${QMAKE_FILE_IN} && \
	$$VUOLINK $$VUOLINK_FLAGS --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN_BASE}.bc
QMAKE_EXTRA_COMPILERS += example
