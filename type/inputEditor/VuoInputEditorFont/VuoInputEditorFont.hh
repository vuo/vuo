/**
 * @file
 * VuoInputEditorFont interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoInputEditor.hh"

extern "C"
{
#include "VuoFont.h"
}

/**
 * A VuoInputEditorFont factory.
 */
class VuoInputEditorFontFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorFont.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a Font-picker dialog.
 */
class VuoInputEditorFont : public VuoInputEditor
{
	Q_OBJECT

public:
	json_object * show(QPoint portLeftCenter, json_object *originalValue, json_object *details, map<QString, json_object *> portNamesAndValues);

public slots:
	void currentFontChanged(VuoFont font);
};
