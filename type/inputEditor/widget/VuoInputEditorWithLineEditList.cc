/**
 * @file
 * VuoInputEditorWithLineEditList implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorWithLineEditList.hh"
#include "VuoInputEditorIcon.hh"

/**
 * Returns the icon for the add button.
 */
static QIcon * renderAddIcon(void)
{
	return VuoInputEditorIcon::renderIcon(^(QPainter &p){
											  p.setPen(Qt::green);
											  p.setBrush(Qt::green);
											  p.drawEllipse(0, 0, 16, 16);
											  p.setPen(Qt::white);
											  p.setBrush(Qt::white);
											  p.drawRect(3, 6, 10, 4);
											  p.drawRect(6, 3, 4, 10);
										  });
}

/**
 * Returns the icon for the remove buttons.
 */
static QIcon * renderRemoveIcon(void)
{
	return VuoInputEditorIcon::renderIcon(^(QPainter &p){
											  p.setPen(Qt::red);
											  p.setBrush(Qt::red);
											  p.drawEllipse(0, 0, 16, 16);
											  p.setPen(Qt::white);
											  p.setBrush(Qt::white);
											  p.drawRect(3, 6, 10, 4);
										  });
}

const int VuoInputEditorWithLineEditList::horizontalSpacing = 5;  ///< The spacing between widgets along the x axis.
const int VuoInputEditorWithLineEditList::verticalSpacing = 4;  ///< The spacing between widgets along the y axis.

/**
 * Creates an input editor whose @c show function displays a series of line edits.
 */
VuoInputEditorWithLineEditList::VuoInputEditorWithLineEditList(bool allowAddingAndRemovingRows, int lineEditWidth)
	: VuoInputEditorWithDialog()
{
	this->allowAddingAndRemovingRows = allowAddingAndRemovingRows;
	this->lineEditWidth = lineEditWidth;
	addButton = NULL;
	dialogLayout = NULL;
}

/**
 * Adds the line edits and associated widgets to the dialog, and populates the line edits with the dialog's initial value.
 */
void VuoInputEditorWithLineEditList::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	dialogLayout = new QVBoxLayout;
	dialogLayout->setContentsMargins(0,0,0,0);
	dialogLayout->setSizeConstraint(QLayout::SetFixedSize);
	//	dialog.setLayout(dialogLayout);  // This repositions the dialog for some reason. Hence the workaround below.
	QWidget *dialogTopLevelWidget = new QWidget(&dialog);
	dialogTopLevelWidget->setLayout(dialogLayout);
	dialogTopLevelWidget->show();

	if (allowAddingAndRemovingRows)
	{
		QIcon *addIcon = renderAddIcon();
		addButton = new QToolButton;
		addButton->setIcon(*addIcon);
		addButton->adjustSize();
		addButton->setFixedSize(addButton->size());
		addButton->setFocusPolicy( (Qt::FocusPolicy)(addButton->focusPolicy() & ~Qt::TabFocus) );
		connect(addButton, SIGNAL(clicked()), this, SLOT(addRow()));
		delete addIcon;

		QHBoxLayout *addButtonLayout = new QHBoxLayout;
		addButtonLayout->addSpacing(lineEditWidth);
		addButtonLayout->addStretch();
		addButtonLayout->addWidget(addButton);
		addButtonLayout->setSpacing(horizontalSpacing);
		addButtonLayout->setSizeConstraint(QLayout::SetFixedSize);

		dialogLayout->addLayout(addButtonLayout);
	}

	QList<QString> values = convertToLineEditListFormat(originalValue);
	foreach (QString value, values)
	{
		addRow();
		lineEdits.back()->setText(value);
	}

	if (! lineEdits.empty())
	{
		lineEdits.front()->setFocus();
		lineEdits.front()->selectAll();
	}

	updateUI();
}

/**
 * Adds a line edit and its associated widgets at the bottom of the dialog.
 */
void VuoInputEditorWithLineEditList::addRow(void)
{
	QDialog *dialog = getDialog();

	QLineEdit *lineEdit = new QLineEdit;
	lineEdit->adjustSize();
	lineEdit->resize(lineEditWidth, lineEdit->height());
	lineEdit->setFixedSize(lineEdit->size());
	lineEdits.append(lineEdit);

	QLayout *rowLayout = setUpRow(*dialog, lineEdit);
	rowLayouts.append(rowLayout);
	rowLayout->setSizeConstraint(QLayout::SetFixedSize);

	dialogLayout->insertLayout(rowLayouts.size() - 1, rowLayout);

	for (int i = 0; i < rowLayout->count(); ++i)
	{
		QWidget *widgetInRow = rowLayout->itemAt(i)->widget();
		if (widgetInRow)
		{
			if (widgetInRow != lineEdit)
				widgetInRow->setFocusPolicy( (Qt::FocusPolicy)(widgetInRow->focusPolicy() & ~Qt::TabFocus) );

			if (sender())
				widgetInRow->show();  // Needed so the widget will be counted when recalculating the dialog size.
		}
	}

	lineEdit->setFocus();

	updateUI();
}

/**
 * When a remove button is clicked, removes the associated line edit and other widgets from the dialog.
 */
void VuoInputEditorWithLineEditList::removeRow(void)
{
	QAbstractButton *eventSender = dynamic_cast<QAbstractButton *>(sender());
	int index = removeButtons.indexOf(eventSender);

	QLayout *rowLayout = rowLayouts.at(index);
	tearDownRow(rowLayout);

	QLayoutItem *itemInRow;
	while ((itemInRow = rowLayout->takeAt(0)) != NULL)
	{
		if (itemInRow->widget())
		{
			rowLayout->removeWidget( itemInRow->widget() );
			delete itemInRow->widget();
		}
		delete itemInRow;
	}

	dialogLayout->removeItem(rowLayout);
	delete rowLayout;

	rowLayouts.removeAt(index);
	lineEdits.removeAt(index);
	removeButtons.removeAt(index);

	if (index < lineEdits.size())
		lineEdits.at(index)->setFocus();
	else if (index > 0 && index == lineEdits.size())
		lineEdits.back()->setFocus();

	updateUI();
}

/**
 * Creates and returns a layout containing @a lineEdit and any associated widgets.
 *
 * The default implementation of this function optionally adds a remove button to the row.
 * Override this function to customize the widgets associated with each line edit.
 */
QLayout * VuoInputEditorWithLineEditList::setUpRow(QDialog &dialog, QLineEdit *lineEdit)
{
	QHBoxLayout *rowLayout = new QHBoxLayout;
	rowLayout->setSpacing(horizontalSpacing);
	rowLayout->addWidget(lineEdit);

	if (allowAddingAndRemovingRows)
	{
		QIcon *removeIcon = renderRemoveIcon();
		QToolButton *removeButton = new QToolButton;
		removeButton->setIcon(*removeIcon);
		removeButton->adjustSize();
		removeButton->setFixedSize(removeButton->size());
		removeButton->move(lineEditWidth + horizontalSpacing, 0);
		connect(removeButton, SIGNAL(clicked()), this, SLOT(removeRow()));
		removeButtons.append(removeButton);
		delete removeIcon;

		rowLayout->addStretch();
		rowLayout->addWidget(removeButton);
	}

	return rowLayout;
}

/**
 * Performs any cleanup necessary before the line edit and associated widgets are removed
 * from the dialog and deallocated.
 *
 * The default implementation of this function does nothing.
 *
 * @param rowLayout The layout that was returned by setUpRow() when the row was added.
 */
void VuoInputEditorWithLineEditList::tearDownRow(QLayout *rowLayout)
{
}

/**
 * Returns the line edit and associated widgets in the row that is currently at the given index.
 *
 * @param index The row index, numbered from 0.
 * @return The layout that was returned by setUpRow() when the row was added.
 */
QLayout * VuoInputEditorWithLineEditList::getRowAtIndex(int index)
{
	if (index < 0 || index >= rowLayouts.size())
		return NULL;

	return rowLayouts.at(index);
}

/**
 * Adds the widget at the bottom of the dialog.
 */
void VuoInputEditorWithLineEditList::addWidgetToDialog(QWidget *widget)
{
	dialogLayout->addWidget(widget);
	widget->setFocusPolicy( (Qt::FocusPolicy)(widget->focusPolicy() & ~Qt::TabFocus) );
	widget->show();

	updateUI();
}

/**
 * Removes the widget from the dialog.
 *
 * @param widget A widget previously passed to addWidgetToDialog().
 */
void VuoInputEditorWithLineEditList::removeWidgetFromDialog(QWidget *widget)
{
	dialogLayout->removeWidget(widget);

	updateUI();
}

/**
 * Returns the current value in the line edits.
 */
json_object * VuoInputEditorWithLineEditList::getAcceptedValue(void)
{
	return convertFromLineEditListFormat( getLineEditTexts() );
}

/**
 * Returns the current text in the line edits.
 */
QList<QString> VuoInputEditorWithLineEditList::getLineEditTexts(void)
{
	QList<QString> texts;
	foreach (QLineEdit *lineEdit, lineEdits)
	{
		texts.append(lineEdit->text());
	}

	return texts;
}

/**
 * Keeps the dialog in a consistent state after widgets have been modified.
 */
void VuoInputEditorWithLineEditList::updateUI(void)
{
	static_cast<QWidget *>(dialogLayout->parent())->adjustSize();
	getDialog()->adjustSize();

	if (! lineEdits.empty())
	{
		foreach (QLineEdit *lineEdit, lineEdits)
			lineEdit->removeEventFilter(this);

		QLineEdit *firstLineEdit = lineEdits.front();
		QLineEdit *lastLineEdit = lineEdits.back();

		setFirstWidgetInTabOrder(firstLineEdit);
		setLastWidgetInTabOrder(lastLineEdit);
	}
}

/**
 * Returns true.
 */
bool VuoInputEditorWithLineEditList::supportsTabbingBetweenPorts(void)
{
	return true;
}
