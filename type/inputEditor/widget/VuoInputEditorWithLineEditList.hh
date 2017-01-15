/**
 * @file
 * VuoInputEditorWithLineEditList interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORWITHLINEEDITLIST_HH
#define VUOINPUTEDITORWITHLINEEDITLIST_HH

#include "VuoInputEditorWithDialog.hh"

/**
 * A base class for input editors that display a series of line edits (text fields).
 */
class VuoInputEditorWithLineEditList : public VuoInputEditorWithDialog
{
	Q_OBJECT

public:
	VuoInputEditorWithLineEditList(bool allowAddingAndRemovingRows=false, int lineEditWidth=100);
	virtual bool supportsTabbingBetweenPorts(void);

protected:
	virtual void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	virtual QLayout * setUpRow(QDialog &dialog, QLineEdit *lineEdit);
	virtual void tearDownRow(QLayout *rowLayout);
	QLayout * getRowAtIndex(int index);
	void addWidgetToDialog(QWidget *widget);
	void removeWidgetFromDialog(QWidget *widget);
	json_object * getAcceptedValue(void);
	QList<QString> getLineEditTexts(void);

	/**
	 * Returns the text that should appear in each of the line edits to represent @a value.
	 */
	virtual QList<QString> convertToLineEditListFormat(json_object *value) = 0;

	/**
	 * Returns the value represented when the given text appears in the line edits.
	 */
	virtual json_object * convertFromLineEditListFormat(const QList<QString> &lineEditTexts) = 0;

private slots:
	void addRow(void);
	void removeRow(void);

private:
	void updateUI(void);

	bool allowAddingAndRemovingRows;
	int lineEditWidth;
	QList<QLayout *> rowLayouts;  ///< Contains lineEdits and removeButtons.
	QList<QLineEdit *> lineEdits;
	QList<QAbstractButton *> removeButtons;
	QAbstractButton *addButton;
	QVBoxLayout *dialogLayout;

	static const int horizontalSpacing;
	static const int verticalSpacing;
};

#endif
