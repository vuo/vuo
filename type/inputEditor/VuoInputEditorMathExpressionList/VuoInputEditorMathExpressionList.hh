/**
 * @file
 * VuoInputEditorMathExpressionList interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoInputEditorWithLineEditList.hh"

extern "C"
{
	#include "VuoText.h"
	#include "VuoList_VuoText.h"
}

/**
 * A VuoInputEditorMathExpression factory.
 */
class VuoInputEditorMathExpressionFactory : public VuoInputEditorFactory
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorMathExpressionList.json")
	Q_INTERFACES(VuoInputEditorFactory)

public:
	VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor for a list of math expressions. It allows each expression to be edited and
 * expressions to be added and removed.
 */
class VuoInputEditorMathExpressionList : public VuoInputEditorWithLineEditList
{
	Q_OBJECT

public:
	VuoInputEditorMathExpressionList(void);

private:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	QLayout * setUpRow(QDialog &dialog, QLineEdit *lineEdit);
	void tearDownRow(QLayout *rowLayout);
	json_object * getAcceptedValue(void);
	QList<QString> convertToLineEditListFormat(json_object *value);
	json_object * convertFromLineEditListFormat(const QList<QString> &lineEditTexts);
	VuoList_VuoText getExpressionsFromLineEditTexts(const QList<QString> &lineEditTexts);
	QString convertToUserLocale(QString valueInStandardLocale);
	QString convertFromUserLocale(QString valueAsString);
	QChar getDecimalPointInUserLocale(void);
	QChar getListSeparatorInUserLocale(void);
	bool eventFilter(QObject *o, QEvent *event) VuoWarnUnusedResult;

private slots:
	void validate();
	void validateMathExpressionList(bool shouldCheckForAtLeastOneExpression=false);

private:
	bool isMathExpressionListValid(void);

	bool hasError;
	QLabel *errorMessage;
	QMap<QLayout *, QLabel *> errorSymbols;
	QRegExpValidator *validator;
	json_object *originalValue;
};
