TEMPLATE = lib
CONFIG += staticlib json qtCore qtGui VuoPCH VuoPCH_objcxx
TARGET = widget

include(../../../vuo.pri)

SOURCES += \
	dummy.mm \
	VuoComboBox.cc \
	VuoDialogForInputEditor.cc \
	VuoInputEditor.cc \
	VuoInputEditorNamedEnum.cc \
	VuoInputEditorWithEnumMenu.cc \
	VuoInputEditorWithMenu.cc \
	VuoInputEditorWithDialog.cc \
	VuoInputEditorWithLineEdit.cc \
	VuoInputEditorWithLineEditList.cc

HEADERS += \
	VuoComboBox.hh \
	VuoDialogForInputEditor.hh \
	VuoInputEditor.hh \
	VuoInputEditorNamedEnum.hh \
	VuoInputEditorWithEnumMenu.hh \
	VuoInputEditorWithMenu.hh \
	VuoInputEditorWithDialog.hh \
	VuoInputEditorWithLineEdit.hh \
	VuoInputEditorWithLineEditList.hh \
	VuoInputEditorIcon.hh \
	VuoInputEditorMenuItem.hh
