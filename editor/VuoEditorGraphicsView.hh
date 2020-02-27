/**
 * @file
 * VuoEditorGraphicsView interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoHeap.h"

/**
 * Subclass of @c QGraphicsView, as a workaround to catch the keyboard shortcut to display the node library.
 * https://b33p.net/kosada/node/5676
 */
class VuoEditorGraphicsView : public QGraphicsView
{
	Q_OBJECT;

public:
	VuoEditorGraphicsView(QWidget *parent);

	bool pinchZoomInProgress();

signals:
	void viewResized();  ///< Emitted when the view has been re-sized.

private slots:
	void updateColor(bool isDark);

private:
	bool event(QEvent * event) VuoWarnUnusedResult;

	void paintEvent(QPaintEvent *event);
	int framesRenderedSinceProfileLogged;
	double renderDurationSinceProfileLogged;
	double lastProfileLoggedTime;

	bool viewportEvent(QEvent *event) VuoWarnUnusedResult;
	double ignorePinchesSmallerThanX;
	double ignorePinchesStartedLessThanXSecondsAfterDrag;
	double ignorePinchesStartedMoreThanXSecondsAfterTouch;

	qreal accumulatedScale;
	bool gestureDetected;
	qreal offset;
};
