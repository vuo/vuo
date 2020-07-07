/**
 * @file
 * VuoPortPopover interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <dispatch/dispatch.h>
#include <VuoImageResize.h>
class VuoEditorComposition;
class VuoPort;

/**
 * A popover widget for displaying information about a port.
 */
class VuoPortPopover : public QTextBrowser
{
	Q_OBJECT
public:
	explicit VuoPortPopover(VuoPort *port, VuoEditorComposition *composition, QWidget *parent=0);
	~VuoPortPopover();

	// Style
	bool getDetached();
	void detach();

signals:
	void popoverClosedForPort(string portID); ///< Emitted when the popover is closed.
	void popoverDetachedFromPort(string portID); ///< Emitted when the popover is detached from its port.
	void popoverResized(); ///< Emitted when the popover has been resized.

public slots:
	void setWindowLevel(bool top);
	void setWindowLevelAndVisibility(bool top);
	void setCompositionRunning(bool running, bool resetDataValue=true);
	void updateTextAndResize();
	void updateDataValueImmediately(QString value);
	void updateLastEventTimeAndDataValue(bool event, bool data, QString value);
	void incrementDroppedEventCount();

protected:
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);
	void closeEvent(QCloseEvent *event);
	void resizeEvent(QResizeEvent *event);

private slots:
	void updateStyle();
	void helpButtonClicked();

private:
	static const int maxPopoverContentWidth;
	static const int maxPopoverImageWidth;
	static const int maxPopoverImageHeight;
	static const qreal minTextUpdateInterval;
	static const int eventHistoryMaxSize;
	static const int noEventObserved;
	static const int noDisplayableEventTime;
	static const string noDataValueObserved;
	static const string noDisplayableDataValue;

	// Port attributes affecting text contents
	string portID;
	VuoEditorComposition *composition;
	string cachedDataValue;
	qint64 timeOfLastEvent;  ///< Displayed in popover.
	qint64 timeOfLastUpdate;  ///< Used to determine how often the popover's contents refresh.
	bool compositionRunning;
	QQueue<qint64> eventHistory;
	unsigned int eventCount;
	unsigned int droppedEventCount;
	bool allEventsBlocked;
	bool someEventsBlocked;

	// Style
	bool isDetached;

	QTimer *refreshTextTimer;
	dispatch_queue_t popoverTextQueue;

	VuoImageResize imageResize;
	QString mostRecentImage;

	void updateTextAndResizeThreadUnsafe();
	void updateTextThreadUnsafe(bool includeEventIndicator=false);
	QString generateImageCode();
	void resetRefreshTextInterval();
	bool refreshTextTimerFiredSinceLastReset;
	QString generatePortPopoverText(bool includeEventIndicator=false);

	static double getEventIntervalMean(QQueue<qint64> timestamps);
	static double getEventIntervalStdDev(QQueue<qint64> timestamps);

	// User interaction
	bool dragInProgress;
	QPoint positionBeforeDrag;
};
