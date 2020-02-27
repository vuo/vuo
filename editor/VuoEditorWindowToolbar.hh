/**
 * @file
 * VuoEditorWindowToolbar interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/// @{ Stub.
Q_FORWARD_DECLARE_OBJC_CLASS(NSWindow);
Q_FORWARD_DECLARE_OBJC_CLASS(NSColor);
Q_FORWARD_DECLARE_OBJC_CLASS(NSSegmentedControl);
Q_FORWARD_DECLARE_OBJC_CLASS(NSTextField);
Q_FORWARD_DECLARE_OBJC_CLASS(NSTitlebarAccessoryViewController);
Q_FORWARD_DECLARE_OBJC_CLASS(NSView);
Q_FORWARD_DECLARE_OBJC_CLASS(NSViewController);
Q_FORWARD_DECLARE_OBJC_CLASS(VuoEditorFreeTrialButton);
/// @}

/**
 * Represents the toolbar for a VuoEditorWindow.
 */
class VuoEditorWindowToolbar : public QObject
{
	Q_OBJECT

public:
	VuoEditorWindowToolbar(QMainWindow *window, bool isCodeEditor=false);
	~VuoEditorWindowToolbar();
	void update(bool eventsShown, bool zoomedToActualSize, bool zoomedToFit);

	void changeStateToBuildPending();
	void changeStateToBuildInProgress();
	void changeStateToRunning();
	void changeStateToStopInProgress();
	void changeStateToStopped();

	bool isBuildPending();
	bool isBuildInProgress();
	bool isRunning();
	bool isStopInProgress();

	static bool usingOverlayScrollers(void);

private:
	bool buildPending;
	bool buildInProgress;
	bool running;
	bool stopInProgress;

	NSWindow *nsWindow;

	NSImage *runImage;
	NSImage *stopImage;

	QMacToolBar *toolBar;
	QMacToolBarItem *toolbarRunItem;
	QMacToolBarItem *toolbarStopItem;
	QMacToolBarItem *toolbarEventsItem;
	NSView *eventsButton;
	QMacToolBarItem *toolbarZoomItem;
	NSSegmentedControl *zoomButtons;

	QTimer *activityIndicatorTimer;
	unsigned int activityIndicatorFrame;

	NSTextField *titleView;
	NSTitlebarAccessoryViewController *titleViewController;

#ifdef VUO_PRO
#include "pro/VuoEditorWindowToolbar_Pro.hh"
#endif

private slots:
	void updateActivityIndicators(void);
	void updateColor(bool isDark);
};
