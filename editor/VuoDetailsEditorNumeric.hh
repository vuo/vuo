/**
 * @file
 * VuoDetailsEditorNumeric interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoType;

/**
 * A widget for editing the details (suggestedMin, suggestedMax, suggestedStep)
 * associated with a numeric published input port.
 */
class VuoDetailsEditorNumeric: public QWidget
{
	Q_OBJECT
public:
	explicit VuoDetailsEditorNumeric(VuoType *type, QWidget *parent=0);
	json_object * show(QPoint portLeftCenter, json_object *originalDetails);

protected:
	void setUpDialog(QDialog &dialog, json_object *originalDetails);
	void setUpLineEdit(QLineEdit *lineEdit, json_object *originalValue);
	QString convertToLineEditFormat(json_object *value);
	json_object * convertFromLineEditsFormat(const QString &suggestedMinValueAsString,
																	const QString &suggestedMaxValueAsString,
																	const QString &suggestedStepValueAsString);
	json_object * getAcceptedValue(void);

private:
	enum detail
	{
		suggestedMin,
		suggestedMax,
		suggestedStep
	};

	VuoType *type;
	map<detail, QLineEdit *> lineEditForDetail;
	map<detail, QLabel *> labelForDetail;

	QFont getDefaultFont(void);
};
