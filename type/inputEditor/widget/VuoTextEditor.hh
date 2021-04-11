/**
 * @file
 * VuoTextEditor interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoInputEditorWithLineEdit.hh"

/**
 * An input editor that displays a text area for entering plain text.
 *
 * The text area has a fixed width but automatically adjusts its height (up to a point) to fit the text.
 */
class VuoTextEditor : public VuoInputEditorWithDialog
{
	Q_OBJECT

public:
	VuoTextEditor(void);
	bool supportsTabbingBetweenPorts(void);

protected:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	json_object * getAcceptedValue(void);
	QString convertToTextEditFormat(json_object *value);
	json_object * convertFromTextEditFormat(const QString &valueAsString);
	void resizeToFitTextWithBaseline(int baselineWidth=0, int baselineHeight=0);

	bool eventFilter(QObject *object, QEvent *event) VuoWarnUnusedResult;
	bool event(QEvent *event) VuoWarnUnusedResult;

	virtual bool getCodeEditor(json_object *details);

	QPlainTextEdit *textEdit;  ///< The text area widget.
	bool isCodeEditor;  ///< Whether or not to treat the input text as code.

protected slots:
	virtual void resizeToFitText();
};

