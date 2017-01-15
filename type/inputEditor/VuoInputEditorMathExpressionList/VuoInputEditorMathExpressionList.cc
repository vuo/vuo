/**
 * @file
 * VuoInputEditorMathExpressionList implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorMathExpressionList.hh"
#include "VuoInputEditorIcon.hh"

extern "C"
{
	#include "VuoHeap.h"
	#include "VuoMathExpressionList.h"
	#include "VuoMathExpressionParser.h"
}

/**
 * Returns the icon for an error within a line edit.
 */
QPixmap * VuoInputEditorMathExpressionList::renderErrorPixmap(void)
{
	const int size = 12;
	return VuoInputEditorIcon::renderPixmap(^(QPainter &p){
												p.setPen(QPen(QColor(179, 43, 43), 1));
												p.setBrush(QColor(230, 57, 57));
												p.drawEllipse(0, 0, size, size);
												p.setPen(Qt::white);
												p.setBrush(Qt::white);
												p.setFont(QFont(getDefaultFont().family(), 11, QFont::Normal));
												p.drawText(0, 0, size, size, Qt::AlignCenter, "!");
											},
											size, size);
}

/**
 * Constructs a VuoInputEditorMathExpression object.
 */
VuoInputEditor * VuoInputEditorMathExpressionFactory::newInputEditor(void)
{
	return new VuoInputEditorMathExpressionList();
}

/**
 * Constructs a VuoInputEditorMathExpression that does not yet have any widgets.
 */
VuoInputEditorMathExpressionList::VuoInputEditorMathExpressionList(void)
	: VuoInputEditorWithLineEditList(false, 200)
{
	hasError = false;
	errorMessage = NULL;
	originalValue = NULL;

	QString validCharacters = QString(" _0-9a-zA-Z=&|<>!+*/^%?:()");
	validCharacters.append( getDecimalPointInUserLocale() );  // . or ,
	validCharacters.append( getListSeparatorInUserLocale() );  // , or ;
	QString validCharactersEscaped = QRegExp::escape(validCharacters);
	validCharactersEscaped.append("\\-");  // not escaped by QRegExp::escape()
	QRegExp regExp( QString("([%1])*").arg(validCharactersEscaped) );
	validator = new QRegExpValidator(regExp, this);
}

/**
 * Adds to @a dialog a line edit for each math expression in @a originalValue.
 */
void VuoInputEditorMathExpressionList::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	VuoInputEditorWithLineEditList::setUpDialog(dialog, originalValue, details);

	this->originalValue = originalValue;

	dialog.installEventFilter(this);
}

/**
 * Sets up the UI to check for and display errors in the math expressions.
 */
QLayout * VuoInputEditorMathExpressionList::setUpRow(QDialog &dialog, QLineEdit *lineEdit)
{
	QLayout *rowLayout = VuoInputEditorWithLineEditList::setUpRow(dialog, lineEdit);

	QPixmap *errorPixmap = renderErrorPixmap();
	QLabel *errorLabel = new QLabel(lineEdit);
	errorLabel->setPixmap(*errorPixmap);
	errorLabel->adjustSize();
	errorLabel->move(lineEdit->width() - errorLabel->width() - rowLayout->spacing(),
					 (lineEdit->height() - errorLabel->height()) / 2.);
	errorLabel->setVisible(false);
	errorSymbols.insert(rowLayout, errorLabel);
	delete errorPixmap;

	lineEdit->setValidator(validator);
	connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(validateMathExpressionList()));

	return rowLayout;
}

/**
 * Removes references to @a rowLayout in data members.
 */
void VuoInputEditorMathExpressionList::tearDownRow(QLayout *rowLayout)
{
	errorSymbols.remove(rowLayout);
}

/**
 * Returns the current math expressions in the line edits, or the original math expressions from
 * setUpDialog() if the current math expressions are invalid.
 */
json_object * VuoInputEditorMathExpressionList::getAcceptedValue(void)
{
	if (! isMathExpressionListValid())
		return originalValue;  // ... in case the user closed the input editor by clicking away from it.

	return VuoInputEditorWithLineEditList::getAcceptedValue();
}

/**
 * Returns a localized text representation of each of the math expressions encoded in @a value.
 */
QList<QString> VuoInputEditorMathExpressionList::convertToLineEditListFormat(json_object *value)
{
	QList<QString> lineEditTexts;

	VuoMathExpressionList expressionList = VuoMathExpressionList_makeFromJson(value);
	unsigned long expressionCount = VuoListGetCount_VuoText(expressionList.expressions);
	for (unsigned long i = 1; i <= expressionCount; ++i)
	{
		VuoText expression = VuoListGetValue_VuoText(expressionList.expressions, i);
		QString expressionString = convertToUserLocale(expression);
		lineEditTexts.append(expressionString);
	}

	return lineEditTexts;
}

/**
 * Returns the math expressions represented by @a lineEditTexts, the localized text contained in the line edits.
 */
json_object * VuoInputEditorMathExpressionList::convertFromLineEditListFormat(const QList<QString> &lineEditTexts)
{
	VuoList_VuoText expressions = getExpressionsFromLineEditTexts(lineEditTexts);

	VuoMathExpressionList expressionList = VuoMathExpressionList_make(expressions);
	json_object *value = VuoMathExpressionList_getJson(expressionList);

	VuoMathExpressionList_retain(expressionList);
	VuoMathExpressionList_release(expressionList);

	return value;
}

/**
 * Returns the list of math expressions represented by the localized text contained in the line edits.
 */
VuoList_VuoText VuoInputEditorMathExpressionList::getExpressionsFromLineEditTexts(const QList<QString> &lineEditTexts)
{
	VuoList_VuoText expressions = VuoListCreate_VuoText();

	foreach (QString lineEditText, lineEditTexts)
	{
		if (! lineEditText.isEmpty())
		{
			QString expressionString = convertFromUserLocale(lineEditText);
			VuoText expression = VuoText_make(expressionString.toUtf8().constData());
			VuoListAppendValue_VuoText(expressions, expression);
		}
	}

	return expressions;
}

/**
 * Converts @a valueInStandardLocale, a math expression in Vuo's standard locale, to the user's system locale.
 */
QString VuoInputEditorMathExpressionList::convertToUserLocale(QString valueInStandardLocale)
{
	return valueInStandardLocale
			.replace(',', getListSeparatorInUserLocale())
			.replace('.', getDecimalPointInUserLocale());
}

/**
 * Converts @a valueInUserLocale, a math expression in the user's system locale, to Vuo's standard locale.
 */
QString VuoInputEditorMathExpressionList::convertFromUserLocale(QString valueInUserLocale)
{
	return valueInUserLocale
			.replace(getDecimalPointInUserLocale(), '.')
			.replace(getListSeparatorInUserLocale(), ',');
}

/**
 * Returns the decimal point character in the user's system locale.
 */
QChar VuoInputEditorMathExpressionList::getDecimalPointInUserLocale(void)
{
	return QLocale::system().decimalPoint();
}

/**
 * Returns the character to separate function arguments in the user's system locale.
 */
QChar VuoInputEditorMathExpressionList::getListSeparatorInUserLocale(void)
{
	return getDecimalPointInUserLocale() == '.' ? ',' : ';';
}

/**
 * Checks if the current math expressions in the line edits are valid, and updates the UI accordingly.
 */
void VuoInputEditorMathExpressionList::validateMathExpressionList(bool shouldCheckForAtLeastOneExpression)
{
	QString errorMessageString;
	VuoList_VuoInteger errorExpressions = NULL;

	QList<QString> lineEditTexts = getLineEditTexts();
	VuoList_VuoText expressions = getExpressionsFromLineEditTexts(lineEditTexts);
	bool hasNoExpressions = (VuoListGetCount_VuoText(expressions) == 0);

	if (hasNoExpressions)
	{
		if (shouldCheckForAtLeastOneExpression)
			errorMessageString = "There must be at least one math expression.";
	}
	else
	{
		VuoMathExpressionError error = NULL;
		VuoMathExpressionParser_makeFromMultipleExpressions(expressions, &error);
		hasError = (error != NULL);

		if (error)
		{
			errorMessageString = VuoMathExpressionError_getMessage(error);
			errorExpressions = VuoMathExpressionError_getExpressionIndices(error);
			VuoMathExpressionError_free(error);
		}
	}

	VuoRetain(expressions);
	VuoRelease(expressions);

	if (errorMessage)
	{
		removeWidgetFromDialog(errorMessage);
		delete errorMessage;
		errorMessage = NULL;
	}

	foreach (QLabel *errorSymbol, errorSymbols.values())
		errorSymbol->setVisible(false);

	if (! errorMessageString.isEmpty())
	{
		errorMessage = new QLabel(errorMessageString);
		errorMessage->setWordWrap(true);
		addWidgetToDialog(errorMessage);

		if (errorExpressions)
		{
			unsigned long count = VuoListGetCount_VuoInteger(errorExpressions);
			for (unsigned long i = 1; i <= count; ++i)
			{
				int indexIgnoringEmptyStrings = VuoListGetValue_VuoInteger(errorExpressions, i) - 1;

				int index;
				int nonEmptyStringIndex = 0;
				for (index = 0; index < lineEditTexts.size(); ++index)
					if (! lineEditTexts.at(index).isEmpty())
						if (nonEmptyStringIndex++ == indexIgnoringEmptyStrings)
							break;

				QLayout *rowLayout = getRowAtIndex(index);
				QLabel *errorSymbol = errorSymbols.value(rowLayout);
				errorSymbol->setVisible(true);
			}

			VuoRetain(errorExpressions);
			VuoRelease(errorExpressions);
		}
	}
}

/**
 * Returns true if there's at least one math expression and all math expressions are valid.
 */
bool VuoInputEditorMathExpressionList::isMathExpressionListValid(void)
{
	VuoList_VuoText expressions = getExpressionsFromLineEditTexts( getLineEditTexts() );
	bool hasNoExpressions = (VuoListGetCount_VuoText(expressions) == 0);
	VuoRetain(expressions);
	VuoRelease(expressions);
	return ! hasError && ! hasNoExpressions;
}

/**
 * If the current math expressions in the line edits are invalid, prevents the Enter/Return key from
 * closing the dialog.
 */
bool VuoInputEditorMathExpressionList::eventFilter(QObject *o, QEvent *event)
{
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
		{
			validateMathExpressionList(true);
			if (! isMathExpressionListValid())
				return true;
		}
	}

	return VuoInputEditorWithLineEditList::eventFilter(o, event);
}
