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
	VuoInputEditorInteger \
	VuoInputEditorKey \
	VuoInputEditorLoopType \
	VuoInputEditorMathExpressionList \
	VuoInputEditorPoint2d \
	VuoInputEditorPoint3d \
	VuoInputEditorPoint4d \
	VuoInputEditorReal \
	VuoInputEditorRealRegulation \
	VuoInputEditorSizingMode \
	VuoInputEditorText \
	VuoInputEditorWave \
	VuoInputEditorWrapMode

VuoInputEditorBlendMode.depends = widget
VuoInputEditorColor.depends = widget
VuoInputEditorCurve.depends = widget
VuoInputEditorCurveEasing.depends = widget
VuoInputEditorDispersion.depends = widget
VuoInputEditorDisplacement.depends = widget
VuoInputEditorFont.depends = widget
VuoInputEditorInteger.depends = widget
VuoInputEditorKey.depends = widget
VuoInputEditorLoopType.depends = widget
VuoInputEditorMathExpressionList.depends = widget
VuoInputEditorPoint2d.depends = widget
VuoInputEditorPoint3d.depends = widget
VuoInputEditorPoint4d.depends = widget
VuoInputEditorReal.depends = widget
VuoInputEditorRealRegulation.depends = widget
VuoInputEditorSizingMode.depends = widget
VuoInputEditorText.depends = widget
VuoInputEditorWave.depends = widget
VuoInputEditorWrapMode.depends = widget

QMAKE_CLEAN = Makefile
