/**
 * @file
 * VuoDetailsEditorPoint implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoDetailsEditorPoint.hh"
#include "VuoDialogForInputEditor.hh"
#include "VuoRendererFonts.hh"
#include "VuoType.hh"

#include "type.h"

/**
 * Creates a widget for editing the details (suggestedMin, suggestedMax, suggestedStep)
 * associated with a VuoPoint{2d,3d,4d} published input port.
 */
VuoDetailsEditorPoint::VuoDetailsEditorPoint(VuoType *type, QWidget *parent) :
	QWidget(parent)
{
	this->type = type;
}

/**
 * Displays a frameless dialog.
 *
 * Returns a json_object with retain count +1; the caller is responsible for releasing it.
 */
json_object * VuoDetailsEditorPoint::show(QPoint portLeftCenter, json_object *originalDetails)
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
void VuoDetailsEditorPoint::setUpDialog(QDialog &dialog, json_object *originalDetails)
{
	// See https://b33p.net/kosada/node/5724
	const int decimalPrecision = 6;

	QValidator *validator = static_cast<QValidator *>(new QDoubleValidator(NULL));

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

	labelForDetail[suggestedMinX] = new QLabel(&dialog);
	labelForDetail[suggestedMinX]->setText(tr("X Suggested Min"));
	labelForDetail[suggestedMinX]->adjustSize();

	labelForDetail[suggestedMaxX] = new QLabel(&dialog);
	labelForDetail[suggestedMaxX]->setText(tr("X Suggested Max"));
	labelForDetail[suggestedMaxX]->adjustSize();

	labelForDetail[suggestedStepX] = new QLabel(&dialog);
	labelForDetail[suggestedStepX]->setText(tr("X Suggested Step"));
	labelForDetail[suggestedStepX]->adjustSize();

	labelForDetail[suggestedMinY] = new QLabel(&dialog);
	labelForDetail[suggestedMinY]->setText(tr("Y Suggested Min"));
	labelForDetail[suggestedMinY]->adjustSize();

	labelForDetail[suggestedMaxY] = new QLabel(&dialog);
	labelForDetail[suggestedMaxY]->setText(tr("Y Suggested Max"));
	labelForDetail[suggestedMaxY]->adjustSize();

	labelForDetail[suggestedStepY] = new QLabel(&dialog);
	labelForDetail[suggestedStepY]->setText(tr("Y Suggested Step"));
	labelForDetail[suggestedStepY]->adjustSize();

	labelForDetail[suggestedMinZ] = new QLabel(&dialog);
	labelForDetail[suggestedMinZ]->setText(tr("Z Suggested Min"));
	labelForDetail[suggestedMinZ]->adjustSize();

	labelForDetail[suggestedMaxZ] = new QLabel(&dialog);
	labelForDetail[suggestedMaxZ]->setText(tr("Z Suggested Max"));
	labelForDetail[suggestedMaxZ]->adjustSize();

	labelForDetail[suggestedStepZ] = new QLabel(&dialog);
	labelForDetail[suggestedStepZ]->setText(tr("Z Suggested Step"));
	labelForDetail[suggestedStepZ]->adjustSize();

	labelForDetail[suggestedMinW] = new QLabel(&dialog);
	labelForDetail[suggestedMinW]->setText(tr("W Suggested Min"));
	labelForDetail[suggestedMinW]->adjustSize();

	labelForDetail[suggestedMaxW] = new QLabel(&dialog);
	labelForDetail[suggestedMaxW]->setText(tr("W Suggested Max"));
	labelForDetail[suggestedMaxW]->adjustSize();

	labelForDetail[suggestedStepW] = new QLabel(&dialog);
	labelForDetail[suggestedStepW]->setText(tr("W Suggested Step"));
	labelForDetail[suggestedStepW]->adjustSize();

	if (dynamic_cast<QDoubleValidator *>(validator))
		static_cast<QDoubleValidator *>(validator)->setDecimals(decimalPrecision);

	// Setup of X line edits
	lineEditForDetail[suggestedMinX] = new QLineEdit(&dialog);
	lineEditForDetail[suggestedMinX]->setValidator(validator);
	lineEditForDetail[suggestedMaxX] = new QLineEdit(&dialog);
	lineEditForDetail[suggestedMaxX]->setValidator(validator);
	lineEditForDetail[suggestedStepX] = new QLineEdit(&dialog);
	lineEditForDetail[suggestedStepX]->setValidator(validator);

	// Setup of Y line edits
	lineEditForDetail[suggestedMinY] = new QLineEdit(&dialog);
	lineEditForDetail[suggestedMinY]->setValidator(validator);
	lineEditForDetail[suggestedMaxY] = new QLineEdit(&dialog);
	lineEditForDetail[suggestedMaxY]->setValidator(validator);
	lineEditForDetail[suggestedStepY] = new QLineEdit(&dialog);
	lineEditForDetail[suggestedStepY]->setValidator(validator);

	// Setup of Z line edits
	lineEditForDetail[suggestedMinZ] = new QLineEdit(&dialog);
	lineEditForDetail[suggestedMinZ]->setValidator(validator);
	lineEditForDetail[suggestedMaxZ] = new QLineEdit(&dialog);
	lineEditForDetail[suggestedMaxZ]->setValidator(validator);
	lineEditForDetail[suggestedStepZ] = new QLineEdit(&dialog);
	lineEditForDetail[suggestedStepZ]->setValidator(validator);

	// Setup of W line edits
	lineEditForDetail[suggestedMinW] = new QLineEdit(&dialog);
	lineEditForDetail[suggestedMinW]->setValidator(validator);
	lineEditForDetail[suggestedMaxW] = new QLineEdit(&dialog);
	lineEditForDetail[suggestedMaxW]->setValidator(validator);
	lineEditForDetail[suggestedStepW] = new QLineEdit(&dialog);
	lineEditForDetail[suggestedStepW]->setValidator(validator);

	if (this->type->getModuleKey() == "VuoPoint2d")
	{
		setUpLineEdit(lineEditForDetail[suggestedMinX], VuoPoint2d_makeFromJson(suggestedMinValue).x, suggestedMinValue);
		setUpLineEdit(lineEditForDetail[suggestedMaxX], VuoPoint2d_makeFromJson(suggestedMaxValue).x, suggestedMaxValue);
		setUpLineEdit(lineEditForDetail[suggestedStepX], VuoPoint2d_makeFromJson(suggestedStepValue).x, suggestedStepValue);

		setUpLineEdit(lineEditForDetail[suggestedMinY], VuoPoint2d_makeFromJson(suggestedMinValue).y, suggestedMinValue);
		setUpLineEdit(lineEditForDetail[suggestedMaxY], VuoPoint2d_makeFromJson(suggestedMaxValue).y, suggestedMaxValue);
		setUpLineEdit(lineEditForDetail[suggestedStepY], VuoPoint2d_makeFromJson(suggestedStepValue).y, suggestedStepValue);
	}
	else if (this->type->getModuleKey() == "VuoPoint3d")
	{
		setUpLineEdit(lineEditForDetail[suggestedMinX], VuoPoint3d_makeFromJson(suggestedMinValue).x, suggestedMinValue);
		setUpLineEdit(lineEditForDetail[suggestedMaxX], VuoPoint3d_makeFromJson(suggestedMaxValue).x, suggestedMaxValue);
		setUpLineEdit(lineEditForDetail[suggestedStepX], VuoPoint3d_makeFromJson(suggestedStepValue).x, suggestedStepValue);

		setUpLineEdit(lineEditForDetail[suggestedMinY], VuoPoint3d_makeFromJson(suggestedMinValue).y, suggestedMinValue);
		setUpLineEdit(lineEditForDetail[suggestedMaxY], VuoPoint3d_makeFromJson(suggestedMaxValue).y, suggestedMaxValue);
		setUpLineEdit(lineEditForDetail[suggestedStepY], VuoPoint3d_makeFromJson(suggestedStepValue).y, suggestedStepValue);

		setUpLineEdit(lineEditForDetail[suggestedMinZ], VuoPoint3d_makeFromJson(suggestedMinValue).z, suggestedMinValue);
		setUpLineEdit(lineEditForDetail[suggestedMaxZ], VuoPoint3d_makeFromJson(suggestedMaxValue).z, suggestedMaxValue);
		setUpLineEdit(lineEditForDetail[suggestedStepZ], VuoPoint3d_makeFromJson(suggestedStepValue).z, suggestedStepValue);
	}
	else if (this->type->getModuleKey() == "VuoPoint4d")
	{
		setUpLineEdit(lineEditForDetail[suggestedMinX], VuoPoint4d_makeFromJson(suggestedMinValue).x, suggestedMinValue);
		setUpLineEdit(lineEditForDetail[suggestedMaxX], VuoPoint4d_makeFromJson(suggestedMaxValue).x, suggestedMaxValue);
		setUpLineEdit(lineEditForDetail[suggestedStepX], VuoPoint4d_makeFromJson(suggestedStepValue).x, suggestedStepValue);

		setUpLineEdit(lineEditForDetail[suggestedMinY], VuoPoint4d_makeFromJson(suggestedMinValue).y, suggestedMinValue);
		setUpLineEdit(lineEditForDetail[suggestedMaxY], VuoPoint4d_makeFromJson(suggestedMaxValue).y, suggestedMaxValue);
		setUpLineEdit(lineEditForDetail[suggestedStepY], VuoPoint4d_makeFromJson(suggestedStepValue).y, suggestedStepValue);

		setUpLineEdit(lineEditForDetail[suggestedMinZ], VuoPoint4d_makeFromJson(suggestedMinValue).z, suggestedMinValue);
		setUpLineEdit(lineEditForDetail[suggestedMaxZ], VuoPoint4d_makeFromJson(suggestedMaxValue).z, suggestedMaxValue);
		setUpLineEdit(lineEditForDetail[suggestedStepZ], VuoPoint4d_makeFromJson(suggestedStepValue).z, suggestedStepValue);

		setUpLineEdit(lineEditForDetail[suggestedMinW], VuoPoint4d_makeFromJson(suggestedMinValue).w, suggestedMinValue);
		setUpLineEdit(lineEditForDetail[suggestedMaxW], VuoPoint4d_makeFromJson(suggestedMaxValue).w, suggestedMaxValue);
		setUpLineEdit(lineEditForDetail[suggestedStepW], VuoPoint4d_makeFromJson(suggestedStepValue).w, suggestedStepValue);
	}

	// Layout of X fields
	lineEditForDetail[suggestedMinX]->move(labelForDetail[suggestedMinX]->pos().x()+labelForDetail[suggestedMinX]->width()+2*widgetHorizontalSpacing, lineEditForDetail[suggestedMinX]->pos().y());
	labelForDetail[suggestedMinX]->show();

	labelForDetail[suggestedMaxX]->move(0, labelForDetail[suggestedMinX]->pos().y() + labelForDetail[suggestedMinX]->height() + widgetVerticalSpacing);
	lineEditForDetail[suggestedMaxX]->move(labelForDetail[suggestedMinX]->pos().x()+labelForDetail[suggestedMinX]->width()+2*widgetHorizontalSpacing, labelForDetail[suggestedMinX]->pos().y() + labelForDetail[suggestedMinX]->height() + widgetVerticalSpacing);
	labelForDetail[suggestedMaxX]->show();

	labelForDetail[suggestedStepX]->move(0, labelForDetail[suggestedMaxX]->pos().y() + labelForDetail[suggestedMaxX]->height() + widgetVerticalSpacing);
	lineEditForDetail[suggestedStepX]->move(labelForDetail[suggestedMinX]->pos().x()+labelForDetail[suggestedMinX]->width()+2*widgetHorizontalSpacing, labelForDetail[suggestedMaxX]->pos().y() + labelForDetail[suggestedMaxX]->height() + widgetVerticalSpacing);
	labelForDetail[suggestedStepX]->show();

	// Layout of Y fields
	labelForDetail[suggestedMinY]->move(0, labelForDetail[suggestedStepX]->pos().y() + labelForDetail[suggestedStepX]->height() + widgetVerticalSpacing);
	lineEditForDetail[suggestedMinY]->move(labelForDetail[suggestedMinX]->pos().x()+labelForDetail[suggestedMinX]->width()+2*widgetHorizontalSpacing, labelForDetail[suggestedStepX]->pos().y() + labelForDetail[suggestedStepX]->height() + widgetVerticalSpacing);
	labelForDetail[suggestedMinY]->show();

	labelForDetail[suggestedMaxY]->move(0, labelForDetail[suggestedMinY]->pos().y() + labelForDetail[suggestedMinY]->height() + widgetVerticalSpacing);
	lineEditForDetail[suggestedMaxY]->move(labelForDetail[suggestedMinX]->pos().x()+labelForDetail[suggestedMinX]->width()+2*widgetHorizontalSpacing, labelForDetail[suggestedMinY]->pos().y() + labelForDetail[suggestedMinY]->height() + widgetVerticalSpacing);
	labelForDetail[suggestedMaxY]->show();

	labelForDetail[suggestedStepY]->move(0, labelForDetail[suggestedMaxY]->pos().y() + labelForDetail[suggestedMaxY]->height() + widgetVerticalSpacing);
	lineEditForDetail[suggestedStepY]->move(labelForDetail[suggestedMinX]->pos().x()+labelForDetail[suggestedMinX]->width()+2*widgetHorizontalSpacing, labelForDetail[suggestedMaxY]->pos().y() + labelForDetail[suggestedMaxY]->height() + widgetVerticalSpacing);
	labelForDetail[suggestedStepY]->show();

	// Hide Z and W fields by default
	labelForDetail[suggestedMinZ]->hide();
	labelForDetail[suggestedMaxZ]->hide();
	labelForDetail[suggestedStepZ]->hide();
	lineEditForDetail[suggestedMinZ]->hide();
	lineEditForDetail[suggestedMaxZ]->hide();
	lineEditForDetail[suggestedStepZ]->hide();

	labelForDetail[suggestedMinW]->hide();
	labelForDetail[suggestedMaxW]->hide();
	labelForDetail[suggestedStepW]->hide();
	lineEditForDetail[suggestedMinW]->hide();
	lineEditForDetail[suggestedMaxW]->hide();
	lineEditForDetail[suggestedStepW]->hide();

	if ((this->type->getModuleKey() == "VuoPoint3d") || (this->type->getModuleKey() == "VuoPoint4d"))
	{
		// Layout of Z fields
		labelForDetail[suggestedMinZ]->move(0, labelForDetail[suggestedStepY]->pos().y() + labelForDetail[suggestedStepY]->height() + widgetVerticalSpacing);
		lineEditForDetail[suggestedMinZ]->move(labelForDetail[suggestedMinX]->pos().x()+labelForDetail[suggestedMinX]->width()+2*widgetHorizontalSpacing, labelForDetail[suggestedStepY]->pos().y() + labelForDetail[suggestedStepY]->height() + widgetVerticalSpacing);
		labelForDetail[suggestedMinZ]->show();
		lineEditForDetail[suggestedMinZ]->show();

		labelForDetail[suggestedMaxZ]->move(0, labelForDetail[suggestedMinZ]->pos().y() + labelForDetail[suggestedMinZ]->height() + widgetVerticalSpacing);
		lineEditForDetail[suggestedMaxZ]->move(labelForDetail[suggestedMinX]->pos().x()+labelForDetail[suggestedMinX]->width()+2*widgetHorizontalSpacing, labelForDetail[suggestedMinZ]->pos().y() + labelForDetail[suggestedMinZ]->height() + widgetVerticalSpacing);
		labelForDetail[suggestedMaxZ]->show();
		lineEditForDetail[suggestedMaxZ]->show();

		labelForDetail[suggestedStepZ]->move(0, labelForDetail[suggestedMaxZ]->pos().y() + labelForDetail[suggestedMaxZ]->height() + widgetVerticalSpacing);
		lineEditForDetail[suggestedStepZ]->move(labelForDetail[suggestedMinX]->pos().x()+labelForDetail[suggestedMinX]->width()+2*widgetHorizontalSpacing, labelForDetail[suggestedMaxZ]->pos().y() + labelForDetail[suggestedMaxZ]->height() + widgetVerticalSpacing);
		labelForDetail[suggestedStepZ]->show();
		lineEditForDetail[suggestedStepZ]->show();

		if (this->type->getModuleKey() == "VuoPoint4d")
		{
			// Layout of W fields
			labelForDetail[suggestedMinW]->move(0, labelForDetail[suggestedStepZ]->pos().y() + labelForDetail[suggestedStepZ]->height() + widgetVerticalSpacing);
			lineEditForDetail[suggestedMinW]->move(labelForDetail[suggestedMinX]->pos().x()+labelForDetail[suggestedMinX]->width()+2*widgetHorizontalSpacing, labelForDetail[suggestedStepZ]->pos().y() + labelForDetail[suggestedStepZ]->height() + widgetVerticalSpacing);
			labelForDetail[suggestedMinW]->show();
			lineEditForDetail[suggestedMinW]->show();

			labelForDetail[suggestedMaxW]->move(0, labelForDetail[suggestedMinW]->pos().y() + labelForDetail[suggestedMinW]->height() + widgetVerticalSpacing);
			lineEditForDetail[suggestedMaxW]->move(labelForDetail[suggestedMinX]->pos().x()+labelForDetail[suggestedMinX]->width()+2*widgetHorizontalSpacing, labelForDetail[suggestedMinW]->pos().y() + labelForDetail[suggestedMinW]->height() + widgetVerticalSpacing);
			labelForDetail[suggestedMaxW]->show();
			lineEditForDetail[suggestedMaxW]->show();

			labelForDetail[suggestedStepW]->move(0, labelForDetail[suggestedMaxW]->pos().y() + labelForDetail[suggestedMaxW]->height() + widgetVerticalSpacing);
			lineEditForDetail[suggestedStepW]->move(labelForDetail[suggestedMinX]->pos().x()+labelForDetail[suggestedMinX]->width()+2*widgetHorizontalSpacing, labelForDetail[suggestedMaxW]->pos().y() + labelForDetail[suggestedMaxW]->height() + widgetVerticalSpacing);
			labelForDetail[suggestedStepW]->show();
			lineEditForDetail[suggestedStepW]->show();
		}
	}

	dialog.adjustSize();

	// Return focus to the topmost line edit.
	lineEditForDetail[suggestedMinX]->setFocus();
	lineEditForDetail[suggestedMinX]->selectAll();
}

/**
 * Configures the provided line edit to display the dialog's initial value.
 *
 * @param lineEdit The already-initialized QLineEdit widget to configure for the dialog.
 * @param originalValue The double value to display in the text field, if it is to be populated at all.
 * @param populateText A boolean indicating whether to populate the text field.
 */
void VuoDetailsEditorPoint::setUpLineEdit(QLineEdit *lineEdit, double originalValue, bool populateText)
{
	lineEdit->setText(populateText? convertToLineEditFormat(originalValue) : "");
	lineEdit->setFocus();
	lineEdit->selectAll();

	lineEdit->adjustSize();
}

/**
 * Returns the current text in the line edits.
 */
json_object * VuoDetailsEditorPoint::getAcceptedValue(void)
{
	return convertFromLineEditsFormat(lineEditForDetail[suggestedMinX]->text(),
									  lineEditForDetail[suggestedMaxX]->text(),
									  lineEditForDetail[suggestedStepX]->text(),
									  lineEditForDetail[suggestedMinY]->text(),
									  lineEditForDetail[suggestedMaxY]->text(),
									  lineEditForDetail[suggestedStepY]->text(),
									  lineEditForDetail[suggestedMinZ]->text(),
									  lineEditForDetail[suggestedMaxZ]->text(),
									  lineEditForDetail[suggestedStepZ]->text(),
									  lineEditForDetail[suggestedMinW]->text(),
									  lineEditForDetail[suggestedMaxW]->text(),
									  lineEditForDetail[suggestedStepW]->text()
									  );
}

/**
 * Returns the text that should appear in the line edit to represent @c value.
 */
QString VuoDetailsEditorPoint::convertToLineEditFormat(double value)
{
	QString valueAsStringInUserLocale = QLocale::system().toString(value);

	if (qAbs(value) >= 1000.0)
		valueAsStringInUserLocale.remove(QLocale::system().groupSeparator());

	return valueAsStringInUserLocale;
}

/**
 * Formats the value from the line edit to conform to the JSON specification for numbers.
 */
json_object * VuoDetailsEditorPoint::convertFromLineEditsFormat(const QString &suggestedMinXValueAsString,
																const QString &suggestedMaxXValueAsString,
																const QString &suggestedStepXValueAsString,
																const QString &suggestedMinYValueAsString,
																const QString &suggestedMaxYValueAsString,
																const QString &suggestedStepYValueAsString,
																const QString &suggestedMinZValueAsString,
																const QString &suggestedMaxZValueAsString,
																const QString &suggestedStepZValueAsString,
																const QString &suggestedMinWValueAsString,
																const QString &suggestedMaxWValueAsString,
																const QString &suggestedStepWValueAsString
																)
{
	// suggestedMin value
	QString suggestedMinXValueJSON = formatDoubleForJSON(suggestedMinXValueAsString);
	QString suggestedMinYValueJSON = formatDoubleForJSON(suggestedMinYValueAsString);
	QString suggestedMinZValueJSON = formatDoubleForJSON(suggestedMinZValueAsString);
	QString suggestedMinWValueJSON = formatDoubleForJSON(suggestedMinWValueAsString);

	// suggestedMax value
	QString suggestedMaxXValueJSON = formatDoubleForJSON(suggestedMaxXValueAsString);
	QString suggestedMaxYValueJSON = formatDoubleForJSON(suggestedMaxYValueAsString);
	QString suggestedMaxZValueJSON = formatDoubleForJSON(suggestedMaxZValueAsString);
	QString suggestedMaxWValueJSON = formatDoubleForJSON(suggestedMaxWValueAsString);

	// suggestedStep value
	QString suggestedStepXValueJSON = formatDoubleForJSON(suggestedStepXValueAsString);
	QString suggestedStepYValueJSON = formatDoubleForJSON(suggestedStepYValueAsString);
	QString suggestedStepZValueJSON = formatDoubleForJSON(suggestedStepZValueAsString);
	QString suggestedStepWValueJSON = formatDoubleForJSON(suggestedStepWValueAsString);

	// details
	struct json_object *details = json_object_new_object();

	auto makeRealFromString = [](const QString &realAsString)
	{
		return VuoMakeRetainedFromString(realAsString.toUtf8().constData(), VuoReal);
	};

	if (this->type->getModuleKey() == "VuoPoint2d")
	{
		if (!suggestedMinXValueAsString.isEmpty() || !suggestedMinYValueAsString.isEmpty())
		{
			VuoPoint2d point;
			point.x = makeRealFromString(suggestedMinXValueJSON);
			point.y = makeRealFromString(suggestedMinYValueJSON);
			json_object_object_add(details, "suggestedMin", VuoPoint2d_getJson(point));
		}
		if (!suggestedMaxXValueAsString.isEmpty() || !suggestedMaxYValueAsString.isEmpty())
		{
			VuoPoint2d point;
			point.x = makeRealFromString(suggestedMaxXValueJSON);
			point.y = makeRealFromString(suggestedMaxYValueJSON);
			json_object_object_add(details, "suggestedMax", VuoPoint2d_getJson(point));
		}
		if (!suggestedStepXValueAsString.isEmpty() || !suggestedStepYValueAsString.isEmpty())
		{
			VuoPoint2d point;
			point.x = makeRealFromString(suggestedStepXValueJSON);
			point.y = makeRealFromString(suggestedStepYValueJSON);
			json_object_object_add(details, "suggestedStep", VuoPoint2d_getJson(point));
		}
	}

	else if (this->type->getModuleKey() == "VuoPoint3d")
	{
		if (!suggestedMinXValueAsString.isEmpty() || !suggestedMinYValueAsString.isEmpty() ||
				!suggestedMinZValueAsString.isEmpty())
		{
			VuoPoint3d point;
			point.x = makeRealFromString(suggestedMinXValueJSON);
			point.y = makeRealFromString(suggestedMinYValueJSON);
			point.z = makeRealFromString(suggestedMinZValueJSON);
			json_object_object_add(details, "suggestedMin", VuoPoint3d_getJson(point));
		}
		if (!suggestedMaxXValueAsString.isEmpty() || !suggestedMaxYValueAsString.isEmpty() ||
				!suggestedMaxZValueAsString.isEmpty())
		{
			VuoPoint3d point;
			point.x = makeRealFromString(suggestedMaxXValueJSON);
			point.y = makeRealFromString(suggestedMaxYValueJSON);
			point.z = makeRealFromString(suggestedMaxZValueJSON);
			json_object_object_add(details, "suggestedMax", VuoPoint3d_getJson(point));
		}
		if (!suggestedStepXValueAsString.isEmpty() || !suggestedStepYValueAsString.isEmpty() ||
				!suggestedStepZValueAsString.isEmpty())
		{
			VuoPoint3d point;
			point.x = makeRealFromString(suggestedStepXValueJSON);
			point.y = makeRealFromString(suggestedStepYValueJSON);
			point.z = makeRealFromString(suggestedStepZValueJSON);
			json_object_object_add(details, "suggestedStep", VuoPoint3d_getJson(point));
		}
	}

	else if (this->type->getModuleKey() == "VuoPoint4d")
	{
		if (!suggestedMinXValueAsString.isEmpty() || !suggestedMinYValueAsString.isEmpty() ||
				!suggestedMinZValueAsString.isEmpty() || !suggestedMinWValueAsString.isEmpty())
		{
			VuoPoint4d point;
			point.x = makeRealFromString(suggestedMinXValueJSON);
			point.y = makeRealFromString(suggestedMinYValueJSON);
			point.z = makeRealFromString(suggestedMinZValueJSON);
			point.w = makeRealFromString(suggestedMinWValueJSON);
			json_object_object_add(details, "suggestedMin", VuoPoint4d_getJson(point));
		}
		if (!suggestedMaxXValueAsString.isEmpty() || !suggestedMaxYValueAsString.isEmpty() ||
				!suggestedMaxZValueAsString.isEmpty() || !suggestedMaxWValueAsString.isEmpty())
		{
			VuoPoint4d point;
			point.x = makeRealFromString(suggestedMaxXValueJSON);
			point.y = makeRealFromString(suggestedMaxYValueJSON);
			point.z = makeRealFromString(suggestedMaxZValueJSON);
			point.w = makeRealFromString(suggestedMaxWValueJSON);
			json_object_object_add(details, "suggestedMax", VuoPoint4d_getJson(point));
		}
		if (!suggestedStepXValueAsString.isEmpty() || !suggestedStepYValueAsString.isEmpty() ||
				!suggestedStepZValueAsString.isEmpty() || !suggestedStepWValueAsString.isEmpty())
		{
			VuoPoint4d point;
			point.x = makeRealFromString(suggestedStepXValueJSON);
			point.y = makeRealFromString(suggestedStepYValueJSON);
			point.z = makeRealFromString(suggestedStepZValueJSON);
			point.w = makeRealFromString(suggestedStepWValueJSON);
			json_object_object_add(details, "suggestedStep", VuoPoint4d_getJson(point));
		}
	}

	return details;
}

/**
 * Formats the input @c doubleString to conform to the JSON specification for numbers.
 */
QString VuoDetailsEditorPoint::formatDoubleForJSON(QString doubleString)
{
	double value = QLocale::system().toDouble(doubleString);
	QString formattedDouble = QLocale(QLocale::C).toString(value);

	if (qAbs(value) >= 1000.0)
		formattedDouble.remove(QLocale(QLocale::C).groupSeparator());

	if (! formattedDouble.isEmpty() && formattedDouble[0] == '.')
		formattedDouble = "0" + formattedDouble;

	return formattedDouble;
}

/**
 * Returns the font that input editors are recommended to use.
 */
QFont VuoDetailsEditorPoint::getDefaultFont(void)
{
	VuoRendererFonts *fonts = VuoRendererFonts::getSharedFonts();
	return fonts->dialogBodyFont();
}
