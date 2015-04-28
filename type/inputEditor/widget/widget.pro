TEMPLATE = lib
CONFIG += staticlib json qtCore qtGui VuoPCH
TARGET = widget

include(../../../vuo.pri)

SOURCES += \
	VuoDialogForInputEditor.cc \
	VuoInputEditor.cc \
	VuoInputEditorWithMenu.cc \
	VuoInputEditorWithDialog.cc \
	VuoInputEditorWithLineEdit.cc

HEADERS += \
	VuoDialogForInputEditor.hh \
	VuoInputEditor.hh \
	VuoInputEditorWithMenu.hh \
	VuoInputEditorWithDialog.hh \
	VuoInputEditorWithLineEdit.hh \
	VuoInputEditorMenuItem.hh
