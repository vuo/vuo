/**
 * @file
 * VuoInputEditorText interface.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORTEXT_HH
#define VUOINPUTEDITORTEXT_HH

#include "VuoInputEditorWithLineEdit.hh"

/**
 * A VuoInputEditorText factory.
 */
class VuoInputEditorTextFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorText.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a line edit and, if the text is outside the bounds
 * of its suggested minimum and maximum length, a warning message.
 *
 * This input editor recognizes the following keys in the JSON details object:
 *   - "suggestedMinLength" and "suggestedMaxLength" define the range of recommended lengths for the
 *		text entered. If the text has fewer or more characters, a warning message is displayed below
 *		the line edit.
 *
 * @eg{
 *   {
 *     "suggestedMinLength" : 7,
 *     "suggestedMaxLength" : 15
 *   }
 * }
 */
class VuoInputEditorText : public VuoInputEditorWithLineEdit
{
	Q_OBJECT

protected:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	QString convertToLineEditFormat(json_object *value);
	json_object * convertFromLineEditFormat(const QString &valueAsString);

	bool eventFilter(QObject *object, QEvent *event);
	bool event(QEvent *event);
};

#endif // VUOINPUTEDITORTEXT_HH
