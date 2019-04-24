/**
 * @file
 * VuoInputEditorBlackmagicOutputDevice interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoInputEditorWithLineEdit.hh"

#include <dlfcn.h>

class VuoComboBox;

/**
 * A VuoInputEditorBlackmagicOutputDevice factory.
 */
class VuoInputEditorBlackmagicOutputDeviceFactory : public VuoInputEditorFactory
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorBlackmagicOutputDevice.json")
	Q_INTERFACES(VuoInputEditorFactory)

public:
	virtual VuoInputEditor *newInputEditor(void);
};

/**
 * An input editor that displays widgets for editing a VuoBlackmagicOutputDevice value,
 * allowing the user to select the device, connection, and video mode from drop-down menus.
 */
class VuoInputEditorBlackmagicOutputDevice : public VuoInputEditorWithDialog
{
	Q_OBJECT

protected:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	json_object *getAcceptedValue(void);

private:
	VuoComboBox *deviceComboBox;
	VuoComboBox *connectionComboBox;
	VuoComboBox *videoModeComboBox;
	void updateConnectionAndModeStatus();

private slots:
	void emitValueChanged(int index);
};
