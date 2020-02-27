/**
 * @file
 * VuoPopover implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoPopover.hh"

#ifdef __APPLE__
#include <objc/runtime.h>
#endif

/**
 * Creates a VuoPopover.
 */
VuoPopover::VuoPopover(QWidget *parent) :
	QWidget(parent)
{
}

int VuoPopover::popoverArrowHalfWidth = 12; ///< Half the width (or exactly the height) of the popover's arrow.

/**
 * Returns a path that represents the outline of a popover that has an arrow on @c arrowSide at @c arrowPixelsFromTopOrLeft.
 *
 * @c arrowPixelsFromTopOrLeft should be between @c cornerRadius and `width() or height() - cornerRadius`.
 * If the provided value is out-of-bounds, the arrow is omitted.
 */
QPainterPath VuoPopover::getPopoverPath(Qt::AnchorPoint arrowSide, int arrowPixelsFromTopOrLeft)
{
	int cornerRadius = 8;

	QPainterPath path;

	bool omitArrow = ((arrowPixelsFromTopOrLeft < cornerRadius) ||
					  (arrowPixelsFromTopOrLeft > (((arrowSide == Qt::AnchorTop) ||
													(arrowSide == Qt::AnchorBottom))?
													   width() :
													   height())
													- cornerRadius));

	if (arrowSide == Qt::AnchorTop)
	{
		path.moveTo(arrowPixelsFromTopOrLeft-popoverArrowHalfWidth,popoverArrowHalfWidth);

		if (!omitArrow)
			path.lineTo(arrowPixelsFromTopOrLeft,0);

		path.lineTo(arrowPixelsFromTopOrLeft+popoverArrowHalfWidth,popoverArrowHalfWidth);
		path.lineTo(width()-cornerRadius,popoverArrowHalfWidth);
		path.cubicTo(width(),popoverArrowHalfWidth, width(),popoverArrowHalfWidth, width(),popoverArrowHalfWidth+cornerRadius);
		path.lineTo(width(),height()-cornerRadius);
		path.cubicTo(width(),height(), width(),height(), width()-cornerRadius,height());
		path.lineTo(cornerRadius,height());
		path.cubicTo(0,height(), 0,height(), 0,height()-cornerRadius);
		path.lineTo(0,popoverArrowHalfWidth+cornerRadius);
		path.cubicTo(0,popoverArrowHalfWidth, 0,popoverArrowHalfWidth, cornerRadius,popoverArrowHalfWidth);
		path.closeSubpath();
	}
	else if (arrowSide == Qt::AnchorRight)
	{
		path.moveTo(width()-popoverArrowHalfWidth-cornerRadius,0);
		path.cubicTo(width()-popoverArrowHalfWidth,0, width()-popoverArrowHalfWidth,0, width()-popoverArrowHalfWidth,cornerRadius);
		path.lineTo(width()-popoverArrowHalfWidth,arrowPixelsFromTopOrLeft-popoverArrowHalfWidth);

		if (!omitArrow)
			path.lineTo(width(),arrowPixelsFromTopOrLeft);

		path.lineTo(width()-popoverArrowHalfWidth,arrowPixelsFromTopOrLeft+popoverArrowHalfWidth);
		path.lineTo(width()-popoverArrowHalfWidth,height()-cornerRadius);
		path.cubicTo(width()-popoverArrowHalfWidth,height(), width()-popoverArrowHalfWidth,height(), width()-popoverArrowHalfWidth-cornerRadius,height());
		path.lineTo(cornerRadius,height());
		path.cubicTo(0,height(), 0,height(), 0,height()-cornerRadius);
		path.lineTo(0,cornerRadius);
		path.cubicTo(0,0, 0,0, cornerRadius,0);
		path.closeSubpath();
	}
	else if (arrowSide == Qt::AnchorBottom)
	{
		path.moveTo(width()-cornerRadius,0);
		path.cubicTo(width(),0, width(),0, width(),cornerRadius);
		path.lineTo(width(),height()-popoverArrowHalfWidth-cornerRadius);
		path.cubicTo(width(),height()-popoverArrowHalfWidth, width(),height()-popoverArrowHalfWidth, width()-cornerRadius,height()-popoverArrowHalfWidth);
		path.lineTo(arrowPixelsFromTopOrLeft+popoverArrowHalfWidth,height()-popoverArrowHalfWidth);

		if (!omitArrow)
			path.lineTo(arrowPixelsFromTopOrLeft,height());

		path.lineTo(arrowPixelsFromTopOrLeft-popoverArrowHalfWidth,height()-popoverArrowHalfWidth);
		path.lineTo(cornerRadius,height()-popoverArrowHalfWidth);
		path.cubicTo(0,height()-popoverArrowHalfWidth, 0,height()-popoverArrowHalfWidth, 0,height()-popoverArrowHalfWidth-cornerRadius);
		path.lineTo(0,cornerRadius);
		path.cubicTo(0,0, 0,0, cornerRadius,0);
		path.closeSubpath();
	}
	else if (arrowSide == Qt::AnchorLeft)
	{
		path.moveTo(width()-cornerRadius,0);
		path.cubicTo(width(),0, width(),0, width(),cornerRadius);
		path.lineTo(width(),height()-cornerRadius);
		path.cubicTo(width(),height(), width(),height(), width()-cornerRadius,height());
		path.lineTo(popoverArrowHalfWidth+cornerRadius,height());
		path.cubicTo(popoverArrowHalfWidth,height(), popoverArrowHalfWidth,height(), popoverArrowHalfWidth,height()-cornerRadius);
		path.lineTo(popoverArrowHalfWidth,arrowPixelsFromTopOrLeft+popoverArrowHalfWidth);

		if (!omitArrow)
			path.lineTo(0,arrowPixelsFromTopOrLeft);

		path.lineTo(popoverArrowHalfWidth,arrowPixelsFromTopOrLeft-popoverArrowHalfWidth);
		path.lineTo(popoverArrowHalfWidth,cornerRadius);
		path.cubicTo(popoverArrowHalfWidth,0, popoverArrowHalfWidth,0, popoverArrowHalfWidth+cornerRadius,0);
		path.closeSubpath();
	}

	return path;
}

/**
 * Returns the recommended content area (inside margins) for a popover with an arrow on @c arrowSide.
 */
QMargins VuoPopover::getPopoverContentsMargins(Qt::AnchorPoint arrowSide)
{
	return QMargins(
				5 + (arrowSide==Qt::AnchorLeft   ? popoverArrowHalfWidth : 0),
				5 + (arrowSide==Qt::AnchorTop    ? popoverArrowHalfWidth : 0),
				5 + (arrowSide==Qt::AnchorRight  ? popoverArrowHalfWidth : 0),
				5 + (arrowSide==Qt::AnchorBottom ? popoverArrowHalfWidth : 0)
												);
}

/**
 * Returns a pointer to the NSWindow associated with the provided widget.
 */
void * VuoPopover::getWindowForPopover(QWidget *popoverWidget)
{
	id view = (id)popoverWidget->winId();

	// window = [view window];
	Class nsView = (Class)objc_getClass("NSView");
	SEL windowSEL = sel_registerName("window");
	Method nsViewWindowMethod = class_getInstanceMethod(nsView, windowSEL);
	IMP nsViewWindow = method_getImplementation(nsViewWindowMethod);
	void *window = nsViewWindow(view, method_getName(nsViewWindowMethod));

	return window;
}

/**
 * Changes the provided popover to float on top of all windows (if @c top is true),
 * or to behave as a normal window (if @c top is false).  If @c top is false,
 * also lowers the popover, effectively hiding it. The change takes effect immediately.
 */
void VuoPopover::setWindowLevelAndVisibility(bool top, QWidget *popoverWidget)
{
	setWindowLevelForNextUpdate(top, popoverWidget);

	if (!top)
		popoverWidget->lower();
}

/**
 * Changes the provided popover to float on top of all windows (if @c top is true),
 * or to behave as a normal window (if @c top is false).  The change takes effect immediately.
 */
void VuoPopover::setWindowLevel(bool top, QWidget *popoverWidget)
{
	setWindowLevelForNextUpdate(top, popoverWidget);

	if (!top)
	{
		// lower()ing forces re-evaluation of the window level, but, unlike raise(),
		// does not cause the application to receive an ApplicationActivate event in Qt 5.3.
		popoverWidget->lower();

		// lower()ing effectively hides the popover; reverse that side effect.
		popoverWidget->hide();
		popoverWidget->show();
	}
}

/**
 * Changes the provided popover to float on top of all windows (if @c top is true),
 * or to behave as a normal window (if @c top is false). Does not force a re-evaluation
 * of the current window level, so the change might not take effect immediately.
 */
void VuoPopover::setWindowLevelForNextUpdate(bool top, QWidget *popoverWidget)
{
	void *window = getWindowForPopover(popoverWidget);

	// [window setLevel:...];
	Class nsWindow = (Class)objc_getClass("NSWindow");
	SEL setLevelSEL = sel_registerName("setLevel:");
	Method nsWindowSetLevelMethod = class_getInstanceMethod(nsWindow, setLevelSEL);
	IMP nsWindowSetLevel = method_getImplementation(nsWindowSetLevelMethod);
	int key;
	if (top)
		key = 8; // kCGMainMenuWindowLevelKey
	else
		key = 0; // Anything higher than zero seems to float..?
	nsWindowSetLevel((id)window, method_getName(nsWindowSetLevelMethod), key);
}
