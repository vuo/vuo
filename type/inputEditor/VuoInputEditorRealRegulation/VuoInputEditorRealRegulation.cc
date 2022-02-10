/**
 * @file
 * VuoInputEditorRealRegulation implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorRealRegulation.hh"

extern "C"
{
	#include "VuoHeap.h"
	#include "VuoRealRegulation.h"
}

/**
 * Constructs a VuoInputEditorRealRegulation object.
 */
VuoInputEditor * VuoInputEditorRealRegulationFactory::newInputEditor(void)
{
	return new VuoInputEditorRealRegulation();
}

/**
 * Constructs a VuoInputEditorRealRegulation that does not yet have any widgets.
 */
VuoInputEditorRealRegulation::VuoInputEditorRealRegulation(void)
	: VuoInputEditorWithLineEditList(false, 100)
{
	row = 0;
}

/**
 * Sets up the row to have a label and a text area.
 */
QLayout *VuoInputEditorRealRegulation::setUpRow(QDialog &dialog, QLineEdit *lineEdit)
{
	QHBoxLayout *rowLayout = static_cast<QHBoxLayout *>(VuoInputEditorWithLineEditList::setUpRow(dialog, lineEdit));

	if (row != 0)
	{
		lineEdit->setValidator(new QDoubleValidator(this));
		lineEdit->setFixedWidth(50);

		QLabel *label = new QLabel(&dialog);
		label->setText(tr("seconds"));
		label->setFont(getDefaultFont());
		if (row != 4)
			label->setStyleSheet("QLabel { color: transparent; }");
		rowLayout->addWidget(label);
	}

	QLabel *label = new QLabel(&dialog);
	const char *texts[] = {"Name", "Minimum", "Maximum", "Default", "Smooth"};
	label->setText(texts[row]);
	label->setFont(getDefaultFont());
	label->setFixedWidth(50);
	rowLayout->insertWidget(0, label);

	++row;
	return rowLayout;
}

/**
 * Returns a text representation of the real regulation encoded in `value`.
 */
QList<QString> VuoInputEditorRealRegulation::convertToLineEditListFormat(json_object *value)
{
	VuoRealRegulation reg = VuoRealRegulation_makeFromJson(value);

	QList<QString> lineEditTexts;
	lineEditTexts.append(reg.name);
	lineEditTexts.append(QLocale().toString(reg.minimumValue));
	lineEditTexts.append(QLocale().toString(reg.maximumValue));
	lineEditTexts.append(QLocale().toString(reg.defaultValue));
	lineEditTexts.append(QLocale().toString(reg.smoothDuration));
	return lineEditTexts;
}

/**
 * Returns the real regulation represented by `lineEditTexts`, the text contained in the line edits.
 */
json_object * VuoInputEditorRealRegulation::convertFromLineEditListFormat(const QList<QString> &lineEditTexts)
{
	VuoRealRegulation reg = VuoRealRegulation_make(
				VuoText_make(lineEditTexts[0].toUtf8().data()),
				QLocale().toDouble(lineEditTexts[1]),
				QLocale().toDouble(lineEditTexts[2]),
				QLocale().toDouble(lineEditTexts[3]),
				QLocale().toDouble(lineEditTexts[4]));

	json_object *value = VuoRealRegulation_getJson(reg);
	VuoRealRegulation_retain(reg);
	VuoRealRegulation_release(reg);
	return value;
}
