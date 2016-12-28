TEMPLATE = lib
CONFIG += staticlib json qtCore qtGui VuoPCH VuoPCH_objcxx
TARGET = widget

include(../../../vuo.pri)

SOURCES += \
	VuoDialogForInputEditor.cc \
	VuoInputEditor.cc \
	VuoInputEditorWithEnumMenu.cc \
	VuoInputEditorWithMenu.cc \
	VuoInputEditorWithDialog.cc \
	VuoInputEditorWithLineEdit.cc \
	VuoInputEditorWithLineEditList.cc

HEADERS += \
	VuoDialogForInputEditor.hh \
	VuoInputEditor.hh \
	VuoInputEditorWithEnumMenu.hh \
	VuoInputEditorWithMenu.hh \
	VuoInputEditorWithDialog.hh \
	VuoInputEditorWithLineEdit.hh \
	VuoInputEditorWithLineEditList.hh \
	VuoInputEditorIcon.hh \
	VuoInputEditorMenuItem.hh
