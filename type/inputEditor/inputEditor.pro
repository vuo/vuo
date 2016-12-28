TEMPLATE = subdirs

SUBDIRS += \
	widget \
	VuoInputEditorBlendMode \
	VuoInputEditorColor \
	VuoInputEditorCurve \
	VuoInputEditorCurveEasing \
	VuoInputEditorDispersion \
	VuoInputEditorDisplacement \
	VuoInputEditorFont \
	VuoInputEditorHidDevice \
	VuoInputEditorInteger \
	VuoInputEditorKey \
	VuoInputEditorLoopType \
	VuoInputEditorMathExpressionList \
	VuoInputEditorMovieFormat \
	VuoInputEditorOscInputDevice \
	VuoInputEditorOscOutputDevice \
	VuoInputEditorPoint2d \
	VuoInputEditorPoint3d \
	VuoInputEditorPoint4d \
	VuoInputEditorReal \
	VuoInputEditorRealRegulation \
	VuoInputEditorScreen \
	VuoInputEditorSerialDevice \
	VuoInputEditorSizingMode \
	VuoInputEditorText \
	VuoInputEditorTransform \
	VuoInputEditorTransform2d \
	VuoInputEditorWave \
	VuoInputEditorWrapMode

VuoInputEditorBlendMode.depends = widget
VuoInputEditorColor.depends = widget
VuoInputEditorCurve.depends = widget
VuoInputEditorCurveEasing.depends = widget
VuoInputEditorDispersion.depends = widget
VuoInputEditorDisplacement.depends = widget
VuoInputEditorFont.depends = widget
VuoInputEditorHidDevice.depends = widget
VuoInputEditorInteger.depends = widget
VuoInputEditorKey.depends = widget
VuoInputEditorLoopType.depends = widget
VuoInputEditorMathExpressionList.depends = widget
VuoInputEditorMovieFormat.depends = widget
VuoInputEditorOscInputDevice.depends = widget
VuoInputEditorOscOutputDevice.depends = widget
VuoInputEditorPoint2d.depends = widget
VuoInputEditorPoint3d.depends = widget
VuoInputEditorPoint4d.depends = widget
VuoInputEditorReal.depends = widget
VuoInputEditorRealRegulation.depends = widget
VuoInputEditorScreen.depends = widget
VuoInputEditorSerialDevice.depends = widget
VuoInputEditorSizingMode.depends = widget
VuoInputEditorText.depends = widget
VuoInputEditorTransform.depends = widget
VuoInputEditorTransform2d.depends = widget
VuoInputEditorWave.depends = widget
VuoInputEditorWrapMode.depends = widget

QMAKE_CLEAN = Makefile
