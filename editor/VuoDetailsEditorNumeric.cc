/**
 * @file
 * VuoDetailsEditorNumeric implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoDetailsEditorNumeric.hh"
#include "VuoDialogForInputEditor.hh"
#include "VuoReal.h"
#include "VuoRendererFonts.hh"
#include "VuoType.hh"

/**
 * Creates a widget for editing the details (suggestedMin, suggestedMax, suggestedStep)
 * associated with a numeric published input port.
 */
VuoDetailsEditorNumeric::VuoDetailsEditorNumeric(VuoType *type, QWidget *parent) :
	QWidget(parent)
{
	this->type = type;
}

/**
 * Displays a frameless dialog.
 *
 * Returns a json_object with retain count +1; the caller is responsible for releasing it.
 */
json_object * VuoDetailsEditorNumeric::show(QPoint portLeftCenter, json_object *originalDetails)
{
	json_object *o = NULL;

	bool isDark = false;
	if (json_object_object_get_ex(originalDetails, "isDark", &o))
		isDark = json_object_get_boolean(o);

	VuoDialogForInputEditor dialog(isDark, false);
	dialog.setFont(getDefaultFont());
	setUpDialog(dialog, originalDetails);

	// Move children to account for margins.
	QMargins margin = dialog.getPopoverContentsMargins();
	QPoint topLeftMargin = QPoint(margin.left(),margin.top());
	foreach (QObject *widget, dialog.children())
		static_cast<QWidget *>(widget)->move(static_cast<QWidget *>(widget)->pos() + topLeftMargin);

	// Resize dialog to enclose child widgets and margins.
	dialog.resize(margin.left() + dialog.childrenRect().width() + margin.right(), margin.top() + dialog.childrenRect().height() + margin.bottom());

	// Position the right center of the dialog at the left center of the port.
	QPoint dialogTopLeft = portLeftCenter - QPoint(dialog.width() - margin.right(), dialog.height()/2.);
	dialog.move(dialogTopLeft);

	dialog.show();  // Needed to position the dialog. (https://bugreports.qt-project.org/browse/QTBUG-31406)
	dialog.exec();

	return (dialog.result() == QDialog::Accepted ? getAcceptedValue() : json_object_get(originalDetails));
}

/**
 * Sets up a dialog containing the port details editor.
 */
void VuoDetailsEditorNumeric::setUpDialog(QDialog &dialog, json_object *originalDetails)
{
	// See https://b33p.net/kosada/node/5724
	const int decimalPrecision = 6;

	QValidator *validator = ((this->type->getModuleKey() == "VuoReal")?
									   static_cast<QValidator *>(new QDoubleValidator(NULL)) :
									   static_cast<QValidator *>(new QIntValidator(NULL)));

	json_object *suggestedMinValue = NULL;
	json_object *suggestedMaxValue = NULL;
	json_object *suggestedStepValue = NULL;

	// Parse supported port annotations from the port's "details" JSON object:
	if (originalDetails)
	{
		// "suggestedMin"
		json_object_object_get_ex(originalDetails, "suggestedMin", &suggestedMinValue);

		// "suggestedMax"
		json_object_object_get_ex(originalDetails, "suggestedMax", &suggestedMaxValue);

		// "suggestedStep"
		json_object_object_get_ex(originalDetails, "suggestedStep", &suggestedStepValue);
	}

	// Layout details
	const int widgetVerticalSpacing = 10;
	const int widgetHorizontalSpacing = 8;

	labelForDetail[suggestedMin] = new QLabel(&dialog);
	labelForDetail[suggestedMin]->setText(tr("Suggested Min"));
	labelForDetail[suggestedMin]->adjustSize();

	labelForDetail[suggestedMax] = new QLabel(&dialog);
	labelForDetail[suggestedMax]->setText(tr("Suggested Max"));
	labelForDetail[suggestedMax]->adjustSize();

	labelForDetail[suggestedStep] = new QLabel(&dialog);
	labelForDetail[suggestedStep]->setText(tr("Suggested Step"));
	labelForDetail[suggestedStep]->adjustSize();

	if (dynamic_cast<QDoubleValidator *>(validator))
		static_cast<QDoubleValidator *>(validator)->setDecimals(decimalPrecision);

	// suggestedMin value
	lineEditForDetail[suggestedMin] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForDetail[suggestedMin], suggestedMinValue);
	lineEditForDetail[suggestedMin]->setValidator(validator);

	// suggestedMax value
	lineEditForDetail[suggestedMax] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForDetail[suggestedMax], suggestedMaxValue);
	lineEditForDetail[suggestedMax]->setValidator(validator);

	// suggestedStep value
	lineEditForDetail[suggestedStep] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForDetail[suggestedStep], suggestedStepValue);
	lineEditForDetail[suggestedStep]->setValidator(validator);

	lineEditForDetail[suggestedMin]->move(labelForDetail[suggestedMin]->pos().x()+labelForDetail[suggestedMin]->width()+2*widgetHorizontalSpacing, lineEditForDetail[suggestedMin]->pos().y());
	labelForDetail[suggestedMin]->show();

	labelForDetail[suggestedMax]->move(0, labelForDetail[suggestedMin]->pos().y() + labelForDetail[suggestedMin]->height() + widgetVerticalSpacing);
	lineEditForDetail[suggestedMax]->move(labelForDetail[suggestedMin]->pos().x()+labelForDetail[suggestedMin]->width()+2*widgetHorizontalSpacing, labelForDetail[suggestedMin]->pos().y() + labelForDetail[suggestedMin]->height() + widgetVerticalSpacing);
	labelForDetail[suggestedMax]->show();

	labelForDetail[suggestedStep]->move(0, labelForDetail[suggestedMax]->pos().y() + labelForDetail[suggestedMax]->height() + widgetVerticalSpacing);
	lineEditForDetail[suggestedStep]->move(labelForDetail[suggestedMin]->pos().x()+labelForDetail[suggestedMin]->width()+2*widgetHorizontalSpacing, labelForDetail[suggestedMax]->pos().y() + labelForDetail[suggestedMax]->height() + widgetVerticalSpacing);
	labelForDetail[suggestedStep]->show();

	dialog.adjustSize();

	// Return focus to the topmost line edit.
	lineEditForDetail[suggestedMin]->setFocus();
	lineEditForDetail[suggestedMin]->selectAll();
}

/**
 * Configures the provided line edit to display the dialog's initial value.
 *
 * @param lineEdit The already-initialized QLineEdit widget to configure for the dialog.
 * @param originalValue The value to display initially in the dialog.
 */
void VuoDetailsEditorNumeric::setUpLineEdit(QLineEdit *lineEdit, json_object *originalValue)
{
	lineEdit->setText( convertToLineEditFormat(originalValue) );
	lineEdit->setFocus();
	lineEdit->selectAll();

	lineEdit->adjustSize();
}

/**
 * Returns the current text in the line edits.
 */
json_object * VuoDetailsEditorNumeric::getAcceptedValue(void)
{
	return convertFromLineEditsFormat(lineEditForDetail[suggestedMin]->text(),
									  lineEditForDetail[suggestedMax]->text(),
									  lineEditForDetail[suggestedStep]->text());
}

/**
 * Returns the text that should appear in the line edit to represent @c value.
 */
QString VuoDetailsEditorNumeric::convertToLineEditFormat(json_object *value)
{
	if (!value)
		return "";

	QString valueAsStringInDefaultLocale = json_object_to_json_string_ext(value, JSON_C_TO_STRING_PLAIN);
	double realValue = ((this->type->getModuleKey() == "VuoReal")?
							QLocale(QLocale::C).toDouble(valueAsStringInDefaultLocale) :
							QLocale(QLocale::C).toInt(valueAsStringInDefaultLocale));
	QString valueAsStringInUserLocale = QLocale::system().toString(realValue);

	if (qAbs(realValue) >= 1000.0)
		valueAsStringInUserLocale.remove(QLocale::system().groupSeparator());

	return valueAsStringInUserLocale;
}

/**
 * Formats the value from the line edit to conform to the JSON specification for numbers.
 */
json_object * VuoDetailsEditorNumeric::convertFromLineEditsFormat(const QString &suggestedMinValueAsString,
																const QString &suggestedMaxValueAsString,
																const QString &suggestedStepValueAsString)
{
	// suggestedMin value
	double suggestedMinValue = ((this->type->getModuleKey() == "VuoReal")?
									QLocale::system().toDouble(suggestedMinValueAsString) :
									QLocale::system().toInt(suggestedMinValueAsString));
	QString suggestedMinValueAsStringInDefaultLocale = QLocale(QLocale::C).toString(suggestedMinValue);

	if (qAbs(suggestedMinValue) >= 1000.0)
		suggestedMinValueAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! suggestedMinValueAsStringInDefaultLocale.isEmpty() && suggestedMinValueAsStringInDefaultLocale[0] == '.')
		suggestedMinValueAsStringInDefaultLocale = "0" + suggestedMinValueAsStringInDefaultLocale;

	// suggestedMax value
	double suggestedMaxValue = ((this->type->getModuleKey() == "VuoReal")?
									QLocale::system().toDouble(suggestedMaxValueAsString) :
									QLocale::system().toInt(suggestedMaxValueAsString));
	QString suggestedMaxValueAsStringInDefaultLocale = QLocale(QLocale::C).toString(suggestedMaxValue);

	if (qAbs(suggestedMaxValue) >= 1000.0)
		suggestedMaxValueAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! suggestedMaxValueAsStringInDefaultLocale.isEmpty() && suggestedMaxValueAsStringInDefaultLocale[0] == '.')
		suggestedMaxValueAsStringInDefaultLocale = "0" + suggestedMaxValueAsStringInDefaultLocale;

	// suggestedStep value
	double suggestedStepValue = ((this->type->getModuleKey() == "VuoReal")?
									 QLocale::system().toDouble(suggestedStepValueAsString) :
									 QLocale::system().toInt(suggestedStepValueAsString));
	QString suggestedStepValueAsStringInDefaultLocale = QLocale(QLocale::C).toString(suggestedStepValue);

	if (qAbs(suggestedStepValue) >= 1000.0)
		suggestedStepValueAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! suggestedStepValueAsStringInDefaultLocale.isEmpty() && suggestedStepValueAsStringInDefaultLocale[0] == '.')
		suggestedStepValueAsStringInDefaultLocale = "0" + suggestedStepValueAsStringInDefaultLocale;

	// details
	struct json_object *details = json_object_new_object();

	if (this->type->getModuleKey() == "VuoReal")
	{
		if (!suggestedMinValueAsString.isEmpty())
			json_object_object_add(details, "suggestedMin", VuoReal_getJson(VuoReal_makeFromString(suggestedMinValueAsStringInDefaultLocale.toUtf8().constData())));

		if (!suggestedMaxValueAsString.isEmpty())
			json_object_object_add(details, "suggestedMax", VuoReal_getJson(VuoReal_makeFromString(suggestedMaxValueAsStringInDefaultLocale.toUtf8().constData())));

		if (!suggestedStepValueAsString.isEmpty())
			json_object_object_add(details, "suggestedStep", VuoReal_getJson(VuoReal_makeFromString(suggestedStepValueAsStringInDefaultLocale.toUtf8().constData())));
	}
	else
	{
		if (!suggestedMinValueAsString.isEmpty())
			json_object_object_add(details, "suggestedMin", VuoInteger_getJson(VuoInteger_makeFromString(suggestedMinValueAsStringInDefaultLocale.toUtf8().constData())));

		if (!suggestedMaxValueAsString.isEmpty())
			json_object_object_add(details, "suggestedMax", VuoInteger_getJson(VuoInteger_makeFromString(suggestedMaxValueAsStringInDefaultLocale.toUtf8().constData())));

		if (!suggestedStepValueAsString.isEmpty())
			json_object_object_add(details, "suggestedStep", VuoInteger_getJson(VuoInteger_makeFromString(suggestedStepValueAsStringInDefaultLocale.toUtf8().constData())));
	}

	return details;
}

/**
 * Returns the font that input editors are recommended to use.
 */
QFont VuoDetailsEditorNumeric::getDefaultFont(void)
{
	VuoRendererFonts *fonts = VuoRendererFonts::getSharedFonts();
	return fonts->dialogBodyFont();
}
