/**
 * @file
 * VuoEditorWindowToolbar implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoEditorWindowToolbar.hh"
#include "ui_VuoEditorWindow.h"

#include "VuoActivityIndicator.hh"
#include "VuoEditor.hh"
#include "VuoEditorWindow.hh"
#include "VuoErrorDialog.hh"
#include "VuoCodeWindow.hh"
#include "VuoCodeEditorStages.hh"

#if defined(slots)
#undef slots
#endif

#include "VuoMacOSSDKWorkaround.h"
#include <AppKit/AppKit.h>

/**
 * A multi-segment button control containing the 4 zoom operations.
 */
@interface VuoEditorZoomButtons : NSSegmentedControl
{
	QMacToolBarItem *_toolBarItem;  ///< The Qt widget corresponding to this Cocoa widget.
	bool _isDark;                   ///< True if dark mode is enabled.
	bool _isCodeEditor;             ///< True if these buttons are for a @ref VuoCodeWindow.
}
@end

@implementation VuoEditorZoomButtons
/**
 * Creates a zoom buttons widget.
 */
- (id)initWithQMacToolBarItem:(QMacToolBarItem *)toolBarItem isCodeEditor:(bool)codeEditor
{
	if (!(self = [super init]))
		return nil;

	_toolBarItem = toolBarItem;
	_isDark = false;
	_isCodeEditor = codeEditor;

	self.accessibilityLabel = [NSString stringWithUTF8String:VuoEditor::tr("Zoom").toUtf8().data()];

	NSImage *zoomOutImage = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"zoom-out" ofType:@"pdf"]];
	[zoomOutImage setTemplate:YES];
	NSImage *zoomFitImage = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"zoom-fit" ofType:@"pdf"]];
	[zoomFitImage setTemplate:YES];
	NSImage *zoomActualImage = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"zoom-actual" ofType:@"pdf"]];
	[zoomActualImage setTemplate:YES];
	NSImage *zoomInImage = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"zoom-in" ofType:@"pdf"]];
	[zoomInImage setTemplate:YES];

	[self setSegmentCount:4];
	int segmentCount = 0;

	[self setImage:zoomOutImage forSegment:segmentCount];
	[self.cell setToolTip:[NSString stringWithUTF8String:VuoEditor::tr("Zoom Out").toUtf8().data()] forSegment:segmentCount];
	[zoomOutImage release];
	++segmentCount;

	[self setImage:zoomActualImage forSegment:segmentCount];
	[self.cell setToolTip:[NSString stringWithUTF8String:VuoEditor::tr("Actual Size").toUtf8().data()] forSegment:segmentCount];
	[zoomActualImage release];
	++segmentCount;

	if (!codeEditor)
	{
		[self setImage:zoomFitImage forSegment:segmentCount];
		[self.cell setToolTip:[NSString stringWithUTF8String:VuoEditor::tr("Zoom to Fit").toUtf8().data()] forSegment:segmentCount];
		[zoomFitImage release];
		++segmentCount;
	}

	[self setImage:zoomInImage forSegment:segmentCount];
	[self.cell setToolTip:[NSString stringWithUTF8String:VuoEditor::tr("Zoom In").toUtf8().data()] forSegment:segmentCount];
	[zoomInImage release];
	++segmentCount;

	[self setSegmentCount:segmentCount];

	[[self cell] setTrackingMode:NSSegmentSwitchTrackingMomentary];

	return self;
}

/**
 * Returns the bounds of the zoom buttons widget.
 */
- (NSRect)bounds
{
	return NSMakeRect(0, 0, [self segmentCount] == 4 ? 149 : 109, 24);
}

/**
 * Cocoa calls this method when a toolbar item is clicked.
 */
- (NSString *)itemIdentifier
{
	if (_isCodeEditor)
	{
		VuoCodeWindow *window = static_cast<VuoCodeWindow *>(_toolBarItem->parent()->parent());
		if ([self isSelectedForSegment:0])
			window->getZoomOutAction()->trigger();
		else if ([self isSelectedForSegment:1])
			window->getZoom11Action()->trigger();
		else if ([self isSelectedForSegment:2])
			window->getZoomInAction()->trigger();
	}
	else
	{
		VuoEditorWindow *window = static_cast<VuoEditorWindow *>(_toolBarItem->parent()->parent());
		if ([self isSelectedForSegment:0])
			window->getZoomOutAction()->trigger();
		else if ([self isSelectedForSegment:1])
			window->getZoom11Action()->trigger();
		else if ([self isSelectedForSegment:2])
			window->getZoomToFitAction()->trigger();
		else if ([self isSelectedForSegment:3])
			window->getZoomInAction()->trigger();
	}

	return QString::number(qulonglong(_toolBarItem)).toNSString();
}

/**
 * Renders the zoom buttons widget.
 */
- (void)drawRect:(NSRect)dirtyRect
{
	NSColor *color = [NSColor colorWithCalibratedWhite:0 alpha:(_isDark ? .1 : .06)];
	[color setStroke];
	[NSBezierPath setDefaultLineWidth:1];
	NSRect rect = [self bounds];

	// Draw a line between each icon, with a 2 pixel margin above/below the icon extents.
	float top = 3;
	float bottom = NSHeight(rect) - 2;
	float right = NSWidth(rect);
	int buttonCount = _isCodeEditor ? 3 : 4;
	for (int i = 1; i < buttonCount; ++i)
		[NSBezierPath strokeLineFromPoint:NSMakePoint(round(right * (float)i/buttonCount) - 2.5, top) toPoint:NSMakePoint(round(right * (float)i/buttonCount) - 2.5, bottom)];

	[[self cell] drawInteriorWithFrame:dirtyRect inView:self];
}

/**
 * Applies dark mode rendering changes.
 */
- (void)updateColor:(bool)isDark
{
	_isDark = isDark;

	// Disable image-templating in dark mode, since it makes the icons too faint.
	int segments = self.segmentCount;
	for (int i = 0; i < segments; ++i)
		[[self imageForSegment:i] setTemplate:!isDark];

	self.needsDisplay = YES;
}
@end


/**
 * A "Show/Hide Events" toggle button.
 */
@interface VuoEditorEventsButton : NSView
{
	QMacToolBarItem *_toolBarItem;  ///< The Qt widget corresponding to this Cocoa widget.
	NSButton *button;               ///< The child button widget.
	bool _isDark;                   ///< True if dark mode is enabled.
}
- (id)initWithQMacToolBarItem:(QMacToolBarItem *)toolBarItem;
- (NSString *)itemIdentifier;
- (void)setState:(bool)state;
- (NSButton *)button;
@end

@implementation VuoEditorEventsButton
/**
 * Creates a show-events toggle button.
 */
- (id)initWithQMacToolBarItem:(QMacToolBarItem *)toolBarItem
{
	if (!(self = [super init]))
		return nil;

	_toolBarItem = toolBarItem;
	_isDark = false;

	button = [NSButton new];
	[button setFrame:NSMakeRect(0,1,32,24)];

	[button setButtonType:NSButtonTypeMomentaryChange];
	[button setBordered:NO];

	[button setAction:@selector(itemIdentifier)];
	[button setTarget:self];

	[[button cell] setImageScaling:NSImageScaleNone];
	[[button cell] setShowsStateBy:NSContentsCellMask];

	button.cell.accessibilityLabel = [NSString stringWithUTF8String:VuoEditor::tr("Show Events").toUtf8().data()];

	// Wrap the button in a fixed-width subview, so the toolbar item doesn't change width when the label changes.
	[self setFrame:NSMakeRect(0,0,32,24)];
	[self addSubview:button];

	return self;
}

/**
 * Returns the bounds of the zoom buttons widget.
 */
- (NSRect)bounds
{
	// So the left side of the Zoom widget lines up with the left side of the canvas.
	double width = 68;

#ifdef VUO_PRO
	VuoEditorEventsButton_Pro();
#endif

	NSRect frame = button.frame;
	frame.origin.x = (width - frame.size.width) / 2.;
	button.frame = frame;

	return NSMakeRect(0, 0, width, 24);
}

/**
 * Cocoa calls this method when a toolbar item is clicked.
 */
- (NSString *)itemIdentifier
{
	VuoEditorWindow *window = static_cast<VuoEditorWindow *>(_toolBarItem->parent()->parent());
	window->getShowEventsAction()->trigger();

	return QString::number(qulonglong(_toolBarItem)).toNSString();
}

/**
 * Toggles the button.
 */
- (void)setState:(bool)state
{
	[button setState:state];

	NSImage *offImage = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"showEvents" ofType:@"pdf"]];
	if (state)
	{
		// Manage state manually, since we need to use NSButtonTypeMomentaryChange to prevent the grey background when clicking.
		NSImage *onImage = [offImage copy];
		[onImage lockFocus];
		[[NSColor colorWithCalibratedRed:29./255 green:106./255 blue:229./255 alpha:1] set];	// #1d6ae5
		NSRectFillUsingOperation(NSMakeRect(0, 0, [onImage size].width, [onImage size].height), NSCompositingOperationSourceAtop);
		[onImage unlockFocus];
		[button setImage:onImage];
		[onImage release];
	}
	else
	{
		[offImage setTemplate:YES];
		[button setImage:offImage];
	}

	[offImage release];

	[self display];
}

/**
 * Returns the button.
 */
- (NSButton *)button
{
	return button;
}

/**
 * Renders the button.
 */
- (void)drawRect:(NSRect)dirtyRect
{
	if (button.state)
	{
		NSColor *color = [NSColor colorWithCalibratedWhite:1 alpha:(_isDark ? .2 : 1)];
		[color setFill];

		NSRect rect = [button frame];
		rect.origin.y -= 1;
		rect.size.height -= 1;
		NSBezierPath *path = [NSBezierPath bezierPathWithRoundedRect:rect xRadius:5.0 yRadius:5.0];
		[path fill];
	}

	[super drawRect:dirtyRect];
}

/**
 * Applies dark mode rendering changes.
 */
- (void)updateColor:(bool)isDark
{
	_isDark = isDark;
	self.needsDisplay = YES;
}
@end

/**
 * Creates a toolbar and adds it to @a window.
 */
VuoEditorWindowToolbar * VuoEditorWindowToolbar::create(QMainWindow *window, bool isCodeEditor)
{
	VuoEditorWindowToolbar *toolbar = new VuoEditorWindowToolbar(window, isCodeEditor);
	toolbar->setUp();
	return toolbar;
}

/**
 * Initializes an empty toolbar.
 */
VuoEditorWindowToolbar::VuoEditorWindowToolbar(QMainWindow *window, bool isCodeEditor)
	: VuoToolbar(window)
{
	this->isCodeEditor = isCodeEditor;

	activityIndicatorTimer = NULL;

	running = false;
	buildInProgress = false;
	buildPending = false;
	stopInProgress = false;

#if VUO_PRO
	VuoEditorWindowToolbar_Pro();
#endif
}

/**
 * Populates the toolbar and adds it to the window.
 */
void VuoEditorWindowToolbar::addToolbarItems(void)
{
	{
		runImage = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"run" ofType:@"pdf"]];
		[runImage setTemplate:YES];

		toolbarRunItem = qtToolbar->addItem(QIcon(), VuoEditor::tr("Run"));
		NSToolbarItem *ti = toolbarRunItem->nativeToolBarItem();
		[ti setImage:runImage];
		[ti setAutovalidates:NO];
		ti.toolTip = [NSString stringWithUTF8String:VuoEditor::tr("Compile and launch this composition.").toUtf8().data()];

		connect(toolbarRunItem, SIGNAL(activated()), window, SLOT(on_runComposition_triggered()));
	}


	{
		stopImage = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"stop" ofType:@"pdf"]];
		[stopImage setTemplate:YES];

		toolbarStopItem = qtToolbar->addItem(QIcon(), VuoEditor::tr("Stop"));
		NSToolbarItem *ti = toolbarStopItem->nativeToolBarItem();
		[ti setImage:stopImage];
		[ti setAutovalidates:NO];
		ti.toolTip = [NSString stringWithUTF8String:VuoEditor::tr("Shut down this composition.").toUtf8().data()];

		connect(toolbarStopItem, SIGNAL(activated()), window, SLOT(on_stopComposition_triggered()));
	}


	if (isCodeEditor)
	{
		eventsButton = NULL;
		toolbarEventsItem = NULL;
		qtToolbar->addSeparator();
	}
	else
	{
		toolbarEventsItem = qtToolbar->addItem(QIcon(), VuoEditor::tr("Show Events"));
		NSToolbarItem *ti = toolbarEventsItem->nativeToolBarItem();
		eventsButton = [[VuoEditorEventsButton alloc] initWithQMacToolBarItem:toolbarEventsItem];
		[ti setView:eventsButton];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		// `setMinSize:` is deprecated with no apparent replacement.
		[ti setMinSize:[eventsButton bounds].size];
#pragma clang diagnostic pop
		ti.toolTip = [NSString stringWithUTF8String:VuoEditor::tr("Toggle whether the canvas shows event flow by highlighting trigger ports and nodes.").toUtf8().data()];
	}


	{
		toolbarZoomItem = qtToolbar->addItem(QIcon(), VuoEditor::tr("Zoom"));
		NSToolbarItem *ti = toolbarZoomItem->nativeToolBarItem();
		zoomButtons = [[VuoEditorZoomButtons alloc] initWithQMacToolBarItem:toolbarZoomItem isCodeEditor:isCodeEditor];
		[ti setView:zoomButtons];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		// `setMinSize:` is deprecated with no apparent replacement.
		[ti setMinSize:[zoomButtons bounds].size];
#pragma clang diagnostic pop
	}


#if VUO_PRO
	addToolbarItems_Pro();
#endif
}

/**
 * Deallocates toolbar data.
 */
VuoEditorWindowToolbar::~VuoEditorWindowToolbar()
{
#if VUO_PRO
	VuoEditorWindowToolbarDestructor_Pro();
#endif
}

/**
 * Always allows tabbing.
 */
bool VuoEditorWindowToolbar::allowsTabbingWithOtherWindows(void)
{
	return true;
}

/**
 * Identifier for grouping composition- and code-editing windows.
 */
NSString * VuoEditorWindowToolbar::getTabbingIdentifier(void)
{
	return @"Vuo Composition";
}

/**
 * Updates the toolbar buttons' states.
 */
void VuoEditorWindowToolbar::update(bool eventsShown, bool zoomedToActualSize, bool zoomedToFit)
{
#if VUO_PRO
	update_Pro();
#endif

	if (!toolbarRunItem)
		return;

	NSToolbarItem *runItem  = toolbarRunItem->nativeToolBarItem();
	NSToolbarItem *stopItem = toolbarStopItem->nativeToolBarItem();

	if (stopInProgress)
	{
		[runItem  setEnabled:!buildPending];
		[stopItem setEnabled:NO];
	}
	else if (buildInProgress)
	{
		[runItem  setEnabled:NO];
		[stopItem setEnabled:YES];
	}
	else if (running)
	{
		[runItem  setEnabled:NO];
		[stopItem setEnabled:YES];
	}
	else
	{
		[runItem  setEnabled:YES];
		[stopItem setEnabled:NO];
	}

	if (toolbarEventsItem)
	{
		NSToolbarItem *eventsItem = toolbarEventsItem->nativeToolBarItem();
		[eventsItem setLabel:[NSString stringWithUTF8String:VuoEditor::tr("Show Events").toUtf8().data()]];
		VuoEditorEventsButton *eventsButton = (VuoEditorEventsButton *)[eventsItem view];
		[eventsButton setState:eventsShown];
	}

	// Enable/disable the zoom11 (actual size) segment.
	[(NSSegmentedControl *)[toolbarZoomItem->nativeToolBarItem() view] setEnabled:!zoomedToActualSize forSegment:1];

	// Enable/disable the "Zoom to Fit" segment.
	[(NSSegmentedControl *)[toolbarZoomItem->nativeToolBarItem() view] setEnabled:!zoomedToFit forSegment:2];

	updateActivityIndicators();
}

/**
 * Indicates that a version of the composition is waiting to build.
 * A previous version may still be stopping at this point.
 */
void VuoEditorWindowToolbar::changeStateToBuildPending()
{
	buildPending = true;
}

/**
 * Advances the state from "build pending" to "build in progress".
 */
void VuoEditorWindowToolbar::changeStateToBuildInProgress()
{
	buildPending = false;
	buildInProgress = true;
}

/**
 * Advances the state from "build in progress" to "running".
 */
void VuoEditorWindowToolbar::changeStateToRunning()
{
	buildInProgress = false;
	running = true;
}

/**
 * Advances the state from "running" to "stop in progress".
 */
void VuoEditorWindowToolbar::changeStateToStopInProgress()
{
	stopInProgress = true;
}

/**
 * Advances the state from "stop in progress" to "stopped".
 */
void VuoEditorWindowToolbar::changeStateToStopped()
{
	buildInProgress = false;
	running = false;
	stopInProgress = false;
}

/**
 * Returns true if the window's composition is waiting to build.
 */
bool VuoEditorWindowToolbar::isBuildPending()
{
	return buildPending;
}

/**
 * Returns true if the window's composition is building.
 */
bool VuoEditorWindowToolbar::isBuildInProgress()
{
	return buildInProgress;
}

/**
 * Returns true if the window's composition is running.
 */
bool VuoEditorWindowToolbar::isRunning()
{
	return running;
}

/**
 * Returns true if the window's composition is stopping.
 */
bool VuoEditorWindowToolbar::isStopInProgress()
{
	return stopInProgress;
}

/**
 * Returns true if System Preferences > General > Show Scroll Bars is set to "When Scrolling" (only available on Mac OS 10.7+).
 * In this mode, scrollbars are drawn as overlays on top of content (instead of reducing the content area).
 */
bool VuoEditorWindowToolbar::usingOverlayScrollers()
{
	return [NSScroller preferredScrollerStyle] == NSScrollerStyleOverlay;
}

/**
 * Updates the animation of the activity indicators for building or stopping the composition (if needed).
 */
void VuoEditorWindowToolbar::updateActivityIndicators(void)
{
	if (buildInProgress || stopInProgress)
	{
		if (! activityIndicatorTimer)
		{
			activityIndicatorFrame = 0;

			activityIndicatorTimer = new QTimer(this);
			activityIndicatorTimer->setObjectName("VuoEditorWindowToolbar::activityIndicatorTimer");
			connect(activityIndicatorTimer, SIGNAL(timeout()), this, SLOT(updateActivityIndicators()));
			activityIndicatorTimer->start(250);
		}
	}
	else
	{
		if (activityIndicatorTimer)
		{
			activityIndicatorTimer->stop();
			delete activityIndicatorTimer;
			activityIndicatorTimer = NULL;
		}
	}

	if (buildInProgress)
	{
		VuoActivityIndicator *iconEngine = new VuoActivityIndicator(activityIndicatorFrame++);
		toolbarRunItem->setIcon(QIcon(iconEngine));
	}
	else
		[(NSToolbarItem *)toolbarRunItem->nativeToolBarItem() setImage:runImage];

	if (stopInProgress)
	{
		VuoActivityIndicator *iconEngine = new VuoActivityIndicator(activityIndicatorFrame++);
		toolbarStopItem->setIcon(QIcon(iconEngine));
	}
	else
		[(NSToolbarItem *)toolbarStopItem->nativeToolBarItem() setImage:stopImage];
}

/**
 * Updates the light/dark styling of the toolbar, including custom button types.
 */
void VuoEditorWindowToolbar::updateColor(bool isDark)
{
	VuoToolbar::updateColor(isDark);

	[(VuoEditorEventsButton *)eventsButton updateColor:isDark];
	[(VuoEditorEventsButton *)zoomButtons updateColor:isDark];
}
