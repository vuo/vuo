/**
 * @file
 * VuoDialogForInputEditor implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoDialogForInputEditor.hh"

/// @todo copied from VuoPopover
const int VuoDialogForInputEditor::popoverArrowHalfWidth = 8; ///< Half the width (or exactly the height) of the popover's arrow.

/// Creates a C string from raw text (saves having to doublequote-escape and/or use backslashes).
#define STRINGIFY(...) QString(#__VA_ARGS__)

/**
 * Initializes the appearance of the dialog.
 */
VuoDialogForInputEditor::VuoDialogForInputEditor(bool isDark, bool showArrow)
{
	setWindowFlags(Qt::FramelessWindowHint /*| Qt::Tool ?*/);

	/// @todo copied from VuoErrorPopover
	// Transparent background behind rounded corners
	setAttribute(Qt::WA_TranslucentBackground, true);

	this->_isDark = isDark;
	this->_showArrow = showArrow;

	if (isDark)
		setStyleSheet(STRINGIFY(
						  * {
							  color: #cfcfcf;
						  }


						  QLineEdit,
						  QDoubleSpinBox,
						  QSpinBox {
							  border: 2px solid #707070;
							  border-radius: 4px;
							  background: #1a1a1a;
							  selection-background-color: #1f365b;
							  selection-color: #ffffff;
						  }


						  QDoubleSpinBox,
						  QSpinBox {
							  padding-right: 12px; /* make room for the arrows */
						  }
						  QDoubleSpinBox::up-button,
						  QSpinBox::up-button {
							  subcontrol-origin: border;
							  subcontrol-position: top right;
							  width: 14px; /* 12px padding + 2px parent border */
							  border: none;
						  }
						  QDoubleSpinBox::up-arrow,
						  QSpinBox::up-arrow {
							  image: url(:/Icons/spinbox-inc.png);
							  width: 12px;
							  height: 12px;
						  }
						  QDoubleSpinBox::down-button,
						  QSpinBox::down-button {
							  subcontrol-origin: border;
							  subcontrol-position: bottom right;
							  width: 14px; /* 12px padding + 2px parent border */
							  border: none;
						  }
						  QDoubleSpinBox::down-arrow,
						  QSpinBox::down-arrow {
							  image: url(:/Icons/spinbox-dec.png);
							  width: 12px;
							  height: 12px;
						  }


						  QSlider::groove:horizontal {
							  height: 4px;
							  background: #404040;
							  border-radius: 2px;
							  margin: 5px 5px;
						  }
						  QSlider::handle:horizontal {
							  background: rgba(32,32,32,224);
							  border: 1px solid #808080;
							  border-radius: 2px;
							  width: 10px;
							  height: 10px;
							  margin: -5px -5px;
						  }
						  ));
}

/**
 * Returns the window's background path.
 */
QPainterPath VuoDialogForInputEditor::getPopoverPath(void)
{
	/// @todo copied from VuoPopover
	int cornerRadius = 8;
	int arrowPixelsFromTopOrLeft = height()/2;
	QPainterPath path;
	path.moveTo(width()-popoverArrowHalfWidth-cornerRadius,0);
	path.cubicTo(width()-popoverArrowHalfWidth,0, width()-popoverArrowHalfWidth,0, width()-popoverArrowHalfWidth,cornerRadius);
	path.lineTo(width()-popoverArrowHalfWidth,arrowPixelsFromTopOrLeft-popoverArrowHalfWidth);
	if (_showArrow)
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
QMargins VuoDialogForInputEditor::getPopoverContentsMargins(void)
{
	return QMargins(5, 5, 5 + popoverArrowHalfWidth, 5);
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
 * If the enter key is pressed, closes the dialog.
 */
void VuoDialogForInputEditor::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
		accept();
	}

	QDialog::keyPressEvent(e);
}

/**
 * If focus is lost (the user has clicked elsewhere), closes the dialog.
 */
bool VuoDialogForInputEditor::event(QEvent *e)
{
	if (e->type() == QEvent::WindowDeactivate)
	{
		accept();
		return true;
	}

	return QDialog::event(e);
}
