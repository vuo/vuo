/**
 * @file
 * VuoDialogForInputEditor implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoDialogForInputEditor.hh"
#include "VuoInputEditor.hh"

/// @todo copied from VuoPopover
const int VuoDialogForInputEditor::popoverArrowHalfWidth = 8; ///< Half the width (or exactly the height) of the popover's arrow.

/**
 * Initializes the appearance of the dialog.
 */
VuoDialogForInputEditor::VuoDialogForInputEditor(bool isDark, bool showArrow)
{
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

	/// @todo copied from VuoErrorPopover
	// Transparent background behind rounded corners
	setAttribute(Qt::WA_TranslucentBackground, true);

	arrowPixelsFromTopOrLeft = 0;

	this->_isDark = isDark;
	this->_showArrow = showArrow;

	respondingToKeypress = false;
}

/**
 * Returns the window's background path.
 */
QPainterPath VuoDialogForInputEditor::getPopoverPath(void)
{
	/// @todo copied from VuoPopover
	int cornerRadius = 8;
	QPainterPath path;
	path.moveTo(width()-popoverArrowHalfWidth-cornerRadius,0);
	path.cubicTo(width()-popoverArrowHalfWidth,0, width()-popoverArrowHalfWidth,0, width()-popoverArrowHalfWidth,cornerRadius);
	path.lineTo(width()-popoverArrowHalfWidth,arrowPixelsFromTopOrLeft-popoverArrowHalfWidth);
	if (_showArrow && arrowPixelsFromTopOrLeft+popoverArrowHalfWidth < height())
		path.lineTo(width(),arrowPixelsFromTopOrLeft);
	path.lineTo(width()-popoverArrowHalfWidth,arrowPixelsFromTopOrLeft+popoverArrowHalfWidth);
	path.lineTo(width()-popoverArrowHalfWidth,height()-cornerRadius);
	path.cubicTo(width()-popoverArrowHalfWidth,height(), width()-popoverArrowHalfWidth,height(), width()-popoverArrowHalfWidth-cornerRadius,height());
	path.lineTo(cornerRadius,height());
	path.cubicTo(0,height(), 0,height(), 0,height()-cornerRadius);
	path.lineTo(0,cornerRadius);
	path.cubicTo(0,0, 0,0, cornerRadius,0);
	path.closeSubpath();

	return path;
}

/**
 * Returns the number of pixels on each side of the window that the input editor should avoid drawing.
 */
QMargins VuoDialogForInputEditor::getPopoverContentsMargins(void) const
{
	return QMargins(5, 5, 5 + popoverArrowHalfWidth, 5);
}

/**
 * Returns the size that this dialog would need in order to enclose its child widgets with a margin.
 * This enables child widgets to call `adjustSize()` on the dialog and preserve the margin.
 */
QSize VuoDialogForInputEditor::sizeHint(void) const
{
	QRect rect = childrenRect();
	if (rect.width() > 0 && rect.height() > 0)
	{
		QRect rectWithMargins = rect.marginsAdded( getPopoverContentsMargins() );
		return rectWithMargins.size();
	}

	return QDialog::sizeHint();
}

/**
 * Draws the popover.
 */
void VuoDialogForInputEditor::paintEvent(QPaintEvent *event)
{
	/// @todo copied from VuoErrorPopover

	QColor backgroundColor = _isDark ? QColor("#505050") : QColor("#f9f9f9");

	QPainter painter(this);

	// Workaround to ensure that transparent background behind rounded corners works consistently
	painter.setCompositionMode(QPainter::CompositionMode_Clear);
	painter.setBrush(backgroundColor);
	painter.drawRect(QRect(0, 0, width(), height()));
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

	painter.setRenderHint(QPainter::Antialiasing, true);

	painter.setPen(Qt::NoPen);
	painter.fillPath(getPopoverPath(), backgroundColor);
}

/**
 * Shows the dialog and sets the popover arrow position.
 */
void VuoDialogForInputEditor::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);

	arrowPixelsFromTopOrLeft = height()/2;
}

/**
 * If the enter key is pressed, closes the dialog.
 */
void VuoDialogForInputEditor::keyPressEvent(QKeyEvent *e)
{
	respondingToKeypress = true;
	if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
		accept();
	}

	QDialog::keyPressEvent(e);
	respondingToKeypress = false;
}

/**
 * If focus is lost (the user has clicked elsewhere), closes the dialog.
 */
bool VuoDialogForInputEditor::event(QEvent *e)
{
	if (e->type() == QEvent::WindowDeactivate)
	{
		// If this deactivation was triggered by a keypress, don't override the
		// accept/reject decision already made in keyPressEvent().
		if (!respondingToKeypress)
			accept();

		return true;
	}

	return QDialog::event(e);
}

/**
 * Sets the boolean indicating whether or not to display an arrow.
 */
void VuoDialogForInputEditor::setShowArrow(bool show)
{
	this->_showArrow = show;
}
