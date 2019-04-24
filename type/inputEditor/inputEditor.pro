TEMPLATE = subdirs

SUBDIRS += \
	widget \
	VuoInputEditorAnchor \
	VuoInputEditorArtNetInputDevice \
	VuoInputEditorArtNetOutputDevice \
	VuoInputEditorAudioInputDevice \
	VuoInputEditorAudioOutputDevice \
	VuoInputEditorBlackmagicInputDevice \
	VuoInputEditorBlackmagicOutputDevice \
	VuoInputEditorBlendMode \
	VuoInputEditorColor \
	VuoInputEditorCurve \
	VuoInputEditorCurveEasing \
	VuoInputEditorDispersion \
	VuoInputEditorDisplacement \
	VuoInputEditorEdgeBlend \
	VuoInputEditorFont \
	VuoInputEditorHidDevice \
	VuoInputEditorInteger \
	VuoInputEditorIntegerRange \
	VuoInputEditorKey \
	VuoInputEditorLoopType \
	VuoInputEditorMathExpressionList \
	VuoInputEditorMidiInputDevice \
	VuoInputEditorMidiOutputDevice \
	VuoInputEditorMovieFormat \
	VuoInputEditorOscInputDevice \
	VuoInputEditorOscOutputDevice \
	VuoInputEditorPoint2d \
	VuoInputEditorPoint3d \
	VuoInputEditorPoint4d \
	VuoInputEditorRange \
	VuoInputEditorReal \
	VuoInputEditorRealRegulation \
	VuoInputEditorScreen \
	VuoInputEditorSerialDevice \
	VuoInputEditorSizingMode \
	VuoInputEditorSyphonServerDescription \
	VuoInputEditorText \
	VuoInputEditorTextComparison \
	VuoInputEditorTransform \
	VuoInputEditorTransform2d \
	VuoInputEditorVideoInputDevice \
	VuoInputEditorWave \
	VuoInputEditorWrapMode

VuoInputEditorAnchor.depends = widget
VuoInputEditorArtNetInputDevice.depends = widget
VuoInputEditorArtNetOutputDevice.depends = widget
VuoInputEditorAudioInputDevice.depends = widget
VuoInputEditorAudioOutputDevice.depends = widget
VuoInputEditorBlackmagicInputDevice.depends = widget
VuoInputEditorBlackmagicOutputDevice.depends = widget
VuoInputEditorBlendMode.depends = widget
VuoInputEditorColor.depends = widget
VuoInputEditorCurve.depends = widget
VuoInputEditorCurveEasing.depends = widget
VuoInputEditorDispersion.depends = widget
VuoInputEditorDisplacement.depends = widget
VuoInputEditorEdgeBlend.depends = widget
VuoInputEditorFont.depends = widget
VuoInputEditorHidDevice.depends = widget
VuoInputEditorInteger.depends = widget
VuoInputEditorIntegerRange.depends = widget
VuoInputEditorKey.depends = widget
VuoInputEditorLoopType.depends = widget
VuoInputEditorMathExpressionList.depends = widget
VuoInputEditorMidiInputDevice.depends = widget
VuoInputEditorMidiOutputDevice.depends = widget
VuoInputEditorMovieFormat.depends = widget
VuoInputEditorOscInputDevice.depends = widget
VuoInputEditorOscOutputDevice.depends = widget
VuoInputEditorPoint2d.depends = widget
VuoInputEditorPoint3d.depends = widget
VuoInputEditorPoint4d.depends = widget
VuoInputEditorRange.depends = widget
VuoInputEditorReal.depends = widget
VuoInputEditorRealRegulation.depends = widget
VuoInputEditorScreen.depends = widget
VuoInputEditorSerialDevice.depends = widget
VuoInputEditorSizingMode.depends = widget
VuoInputEditorSyphonServerDescription.depends = widget
VuoInputEditorText.depends = widget
VuoInputEditorTextComparison.depends = widget
VuoInputEditorTransform.depends = widget
VuoInputEditorTransform2d.depends = widget
VuoInputEditorVideoInputDevice.depends = widget
VuoInputEditorWave.depends = widget
VuoInputEditorWrapMode.depends = widget

QMAKE_CLEAN = Makefile
