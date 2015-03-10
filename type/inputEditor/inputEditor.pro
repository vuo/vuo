TEMPLATE = subdirs

SUBDIRS += \
	widget \
	VuoInputEditorBlendMode \
	VuoInputEditorBoolean \
	VuoInputEditorColor \
	VuoInputEditorCurve \
	VuoInputEditorCurveDomain \
	VuoInputEditorFont \
	VuoInputEditorGradientNoise \
	VuoInputEditorInteger \
	VuoInputEditorImageWrapMode \
	VuoInputEditorLoopType \
	VuoInputEditorModifierKey \
	VuoInputEditorMouseButton \
	VuoInputEditorNoise \
	VuoInputEditorPoint2d \
	VuoInputEditorPoint3d \
	VuoInputEditorPoint4d \
	VuoInputEditorPointableType \
	VuoInputEditorReal \
	VuoInputEditorSizingMode \
	VuoInputEditorText \
	VuoInputEditorThresholdType \
	VuoInputEditorTouchZone \
	VuoInputEditorWave \
	VuoInputEditorWrapMode

VuoInputEditorBlendMode.depends = widget
VuoInputEditorBoolean.depends = widget
VuoInputEditorColor.depends = widget
VuoInputEditorCurve.depends = widget
VuoInputEditorCurveDomain.depends = widget
VuoInputEditorFont.depends = widget
VuoInputEditorGradientNoise.depends = widget
VuoInputEditorInteger.depends = widget
VuoInputEditorImageWrapMode.depends = widget
VuoInputEditorLoopType.depends = widget
VuoInputEditorModifierKey.depends = widget
VuoInputEditorMouseButton.depends = widget
VuoInputEditorNoise.depends = widget
VuoInputEditorPoint2d.depends = widget
VuoInputEditorPoint3d.depends = widget
VuoInputEditorPoint4d.depends = widget
VuoInputEditorPointableType.depends = widget
VuoInputEditorReal.depends = widget
VuoInputEditorSizingMode.depends = widget
VuoInputEditorText.depends = widget
VuoInputEditorThresholdType.depends = widget
VuoInputEditorTouchZone.depends = widget
VuoInputEditorWave.depends = widget
VuoInputEditorWrapMode.depends = widget

QMAKE_CLEAN = Makefile
