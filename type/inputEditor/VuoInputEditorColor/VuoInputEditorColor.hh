/**
 * @file
 * VuoInputEditorColor interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORCOLOR_HH
#define VUOINPUTEDITORCOLOR_HH

#include "VuoInputEditor.hh"

extern "C"
{
	#include "VuoColor.h"
}

/**
 * A VuoInputEditorColor factory.
 */
class VuoInputEditorColorFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorColor.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a color-picker dialog.
 */
class VuoInputEditorColor : public VuoInputEditor
{
	Q_OBJECT

public:
	json_object * show(QPoint portLeftCenter, json_object *originalValue, json_object *details, map<QString, json_object *> portNamesAndValues);

private:
	VuoColor vuoColorFromQColor(const QColor &qtColor);

private slots:
	void currentColorChanged(const QColor &color);
};

#endif // VUOINPUTEDITORCOLOR_HH
