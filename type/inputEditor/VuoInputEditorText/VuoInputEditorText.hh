/**
 * @file
 * VuoInputEditorText interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
 * An input editor that displays a text area for entering plain text.
 *
 * The text area has a fixed width but automatically adjusts its height (up to a point) to fit the text.
 *
 * This input editor recognizes the following keys in the JSON details object:
 *   - "isCodeEditor" — If true, the input editor is adapted for editing source code (wider text area,
 *	   fixed-width font, Return and Tab keys type text instead of dismissing the input editor).
 *     Defaults to false.
 */
class VuoInputEditorText : public VuoInputEditorWithDialog
{
	Q_OBJECT

public:
	VuoInputEditorText(void);
	bool supportsTabbingBetweenPorts(void);

protected:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	json_object * getAcceptedValue(void);
	QString convertToTextEditFormat(json_object *value);
	json_object * convertFromTextEditFormat(const QString &valueAsString);

	bool eventFilter(QObject *object, QEvent *event);
	bool event(QEvent *event);

private slots:
	void resizeToFitText(void);

private:
	QPlainTextEdit *textEdit;  ///< The text area widget.
	bool isCodeEditor;
};

#endif // VUOINPUTEDITORTEXT_HH
