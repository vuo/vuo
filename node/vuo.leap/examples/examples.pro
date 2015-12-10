TEMPLATE = aux

include(../../../vuo.pri)

!equals(MAC_VERSION, "10.6") {

EXAMPLE_SOURCES += \
	DisplayLeapHand.vuo \
	HighlightExtendedFingers.vuo \
	HoldEgg.vuo \
	ShowHandStatus.vuo \
	TwirlImageWithLeap.vuo

}

include(../../../example.pri)
