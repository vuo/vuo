/**
 * @file
 * VuoPopover interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Provides common methods for rendering popover windows.
 */
class VuoPopover : public QWidget
{
	Q_OBJECT
public:
	explicit VuoPopover(QWidget *parent = 0);
	static void * getWindowForPopover(QWidget *popoverWidget);
	static void setWindowLevelAndVisibility(bool top, QWidget *popoverWidget);
	static void setWindowLevel(bool top, QWidget *popoverWidget);

protected:
	static int popoverArrowHalfWidth;
	static void setWindowLevelForNextUpdate(bool top, QWidget *popoverWidget);
	QPainterPath getPopoverPath(Qt::AnchorPoint arrowSide, int arrowPixelsFromTopOrLeft);
	QMargins getPopoverContentsMargins(Qt::AnchorPoint arrowSide);
};
