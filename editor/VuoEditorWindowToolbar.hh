/**
 * @file
 * VuoEditorWindowToolbar interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoToolbar.hh"

/// @{ Stub.
Q_FORWARD_DECLARE_OBJC_CLASS(NSColor);
Q_FORWARD_DECLARE_OBJC_CLASS(NSSegmentedControl);
Q_FORWARD_DECLARE_OBJC_CLASS(NSView);
Q_FORWARD_DECLARE_OBJC_CLASS(NSViewController);
Q_FORWARD_DECLARE_OBJC_CLASS(VuoEditorFreeTrialButton);
/// @}

/**
 * Toolbar for a composition- or code-editing window.
 */
class VuoEditorWindowToolbar : public VuoToolbar
{
	Q_OBJECT

public:
	static VuoEditorWindowToolbar * create(QMainWindow *window, bool isCodeEditor=false);
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

protected:
	VuoEditorWindowToolbar(QMainWindow *window, bool isCodeEditor);
	virtual void addToolbarItems(void) override;
	virtual bool allowsTabbingWithOtherWindows(void) override;
	virtual NSString * getTabbingIdentifier(void) override;
	virtual void updateColor(bool isDark) override;

private:
	bool isCodeEditor;

	bool buildPending;
	bool buildInProgress;
	bool running;
	bool stopInProgress;

	NSImage *runImage;
	NSImage *runImage11;    //  1:1  aspect ratio, to be used when toolbar labels are visible
	NSImage *runImage1611;  // 16:11 aspect ratio, to be used when toolbar labels are hidden

	NSImage *stopImage;
	NSImage *stopImage11;
	NSImage *stopImage1611;

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
};
