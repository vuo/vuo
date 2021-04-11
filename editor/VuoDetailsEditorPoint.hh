/**
 * @file
 * VuoDetailsEditorPoint interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoType;

extern "C" {
#include "VuoPoint2d.h"
#include "VuoPoint3d.h"
#include "VuoPoint4d.h"
}

/**
 * A widget for editing the details (suggestedMin, suggestedMax, suggestedStep)
 * associated with a Point published input port.
 */
class VuoDetailsEditorPoint: public QWidget
{
	Q_OBJECT
public:
	explicit VuoDetailsEditorPoint(VuoType *type, QWidget *parent=0);
	json_object * show(QPoint portLeftCenter, json_object *originalDetails);

protected:
	void setUpDialog(QDialog &dialog, json_object *originalDetails);
	void setUpLineEdit(QLineEdit *lineEdit, double value, bool populateText);
	QString convertToLineEditFormat(double value);
	json_object * convertFromLineEditsFormat(const QString &suggestedMinXValueAsString,
											 const QString &suggestedMaxValueAsString,
											 const QString &suggestedStepXValueAsString,
											 const QString &suggestedMinYValueAsString,
											 const QString &suggestedMaxYValueAsString,
											 const QString &suggestedStepYValueAsString,
											 const QString &suggestedMinZValueAsString,
											 const QString &suggestedMaxZValueAsString,
											 const QString &suggestedStepZValueAsString,
											 const QString &suggestedMinWValueAsString,
											 const QString &suggestedMaxWValueAsString,
											 const QString &suggestedStepWValueAsString);
	json_object * getAcceptedValue(void);

private:
	enum detail
	{
		suggestedMinX,
		suggestedMinY,
		suggestedMinZ,
		suggestedMinW,
		suggestedMaxX,
		suggestedMaxY,
		suggestedMaxZ,
		suggestedMaxW,
		suggestedStepX,
		suggestedStepY,
		suggestedStepZ,
		suggestedStepW
	};

	VuoType *type;
	map<detail, QLineEdit *> lineEditForDetail;
	map<detail, QLabel *> labelForDetail;

	QString formatDoubleForJSON(QString doubleString);
	QFont getDefaultFont(void);
};
