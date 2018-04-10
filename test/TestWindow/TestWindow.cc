/**
 * @file
 * TestWindow implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#include <QtTest/QtTest>
#pragma clang diagnostic pop

#include "../../framework/Vuo.framework/Headers/Vuo.h" // Use this instead of framework-style Vuo/Vuo.h so Qt Creator can find it.
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

/**
 * Performs `action()` and waits for `port` to receive data `expectedValue`.
 */
#define EXPECT_EVENT(port, expectedValue, action) \
	delegate.expectEventOnPort(__LINE__, port, expectedValue, action);

/**
 * Performs `action()` and waits for an event.
 */
#define EXPECT_ANY_EVENT(action) \
	delegate.expectAnyEvent(__LINE__, action);

/**
 * Performs `action()` and ensures no event is output on any published port for 1 second after.
 */
#define EXPECT_NO_EVENT(action) \
	delegate.expectNoEvent(__LINE__, action);

class TestWindowDelegate : public VuoRunnerDelegateAdapter
{
	VuoRunner *runner;
	VuoRunner::Port *expectedPort;
	dispatch_semaphore_t fulfilledExpectation;
	int line;

	string expectedType;
	union
	{
		VuoPoint2d point2d;
		VuoBoolean boolean;
	} expectedValue;

	void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port *actualPort, bool sentData, string actualDataSummary)
	{
//		VLog("%s %s", actualPort?actualPort->getName().c_str():"", actualDataSummary.c_str());

		if (expectedPort)
		{
			if (expectedPort != actualPort)
				QTest::qFail(QString("Expected event on port %1 but got an event on port %2.")
					  .arg(expectedPort->getName().c_str())
					  .arg(actualPort->getName().c_str())
					  .toUtf8().data(), __FILE__, line);

			if (expectedType != actualPort->getType())
				QTest::qFail(QString("Expected event on port %1 to have type %2 but got type %3.")
					  .arg(expectedPort->getName().c_str())
					  .arg(expectedType.c_str())
					  .arg(actualPort->getType().c_str())
					  .toUtf8().data(), __FILE__, line);

			if (expectedType == "VuoPoint2d")
			{
				VuoPoint2d actualPoint = VuoPoint2d_makeFromJson(runner->getPublishedOutputPortValue(actualPort));
				if ( !(fabs(expectedValue.point2d.x - actualPoint.x) < 0.0001
					&& fabs(expectedValue.point2d.y - actualPoint.y) < 0.0001) )
					QTest::qFail(QString("Expected port %1 to have value %2 but got value %3.")
						  .arg(expectedPort->getName().c_str())
						  .arg(VuoPoint2d_getSummary(expectedValue.point2d))
						  .arg(actualDataSummary.c_str())
						  .toUtf8().data(), __FILE__, line);
			}
			else if (expectedType == "VuoBoolean")
			{
				VuoBoolean actualBoolean = VuoBoolean_makeFromJson(runner->getPublishedOutputPortValue(actualPort));
				if (expectedValue.boolean != actualBoolean)
					QTest::qFail(QString("Expected port %1 to have value %2 but got value %3.")
						  .arg(expectedPort->getName().c_str())
						  .arg(VuoBoolean_getSummary(expectedValue.boolean))
						  .arg(actualDataSummary.c_str())
						  .toUtf8().data(), __FILE__, line);
			}
			else
				QTest::qFail(QString("Unknown type %1.")
					  .arg(expectedType.c_str())
					  .toUtf8().data(), __FILE__, line);
		}

		dispatch_semaphore_signal(fulfilledExpectation);
	}

	void expectEvent(void (^action)(void))
	{
		QVERIFY(this->expectedPort);

		dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
			action();
		});

		// 6 seconds, to leave room for Mac OS X's slowest double-click timeout (5 seconds).
		bool timeout = dispatch_semaphore_wait(fulfilledExpectation, dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC*6));
		if (timeout)
			QTest::qFail(QString("Timeout while expecting port %1 to receive an event.")
				  .arg(expectedPort->getName().c_str())
				  .toUtf8().data(), __FILE__, line);

		this->expectedPort = NULL;
	}

public:
	TestWindowDelegate(VuoRunner *runner)
	{
		this->runner = runner;
		fulfilledExpectation = dispatch_semaphore_create(0);
	}
	~TestWindowDelegate()
	{
		dispatch_release(fulfilledExpectation);
	}

	void expectEventOnPort(int line, VuoRunner::Port *port, VuoPoint2d expectedPoint, void (^action)(void))
	{
		this->line = line;
		this->expectedPort = port;
		this->expectedType = "VuoPoint2d";
		this->expectedValue.point2d = expectedPoint;
		expectEvent(action);
	}

	void expectEventOnPort(int line, VuoRunner::Port *port, bool expectedBoolean, void (^action)(void))
	{
		this->line = line;
		this->expectedPort = port;
		this->expectedType = "VuoBoolean";
		this->expectedValue.boolean = expectedBoolean;
		expectEvent(action);
	}

	void expectAnyEvent(int line, void (^action)(void))
	{
		this->line = line;
		this->expectedPort = NULL;

		dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
			action();
		});

		bool timeout = dispatch_semaphore_wait(fulfilledExpectation, dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC*6));
		if (timeout)
			QTest::qFail("Expected an event but didn't get one.", __FILE__, line);
	}

	void expectNoEvent(int line, void (^action)(void))
	{
		this->line = line;
		this->expectedPort = NULL;

		dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
			action();
		});

		bool timeout = dispatch_semaphore_wait(fulfilledExpectation, dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC));
		if (!timeout)
			QTest::qFail("Expected no event, but got an event.", __FILE__, line);
	}
};

/**
 * Tests interacting with composition windows.
 */
class TestWindow : public QObject
{
	Q_OBJECT

private:
	void sendEvents(vector<CGEventRef> events, useconds_t timeBetweenEvents = USEC_PER_SEC/2)
	{
		for (vector<CGEventRef>::iterator event = events.begin(); event != events.end(); ++event)
		{
			CGEventPost(kCGHIDEventTap, *event);
			usleep(timeBetweenEvents);
		}
	}

	void sendFullscreenKeystroke()
	{
		vector<CGEventRef> events;

		events.push_back(CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_Command, true));

		CGEventRef e = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_ANSI_F,  true);
		CGEventSetFlags(e, kCGEventFlagMaskCommand);
		events.push_back(e);

		e = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_ANSI_F,  false);
		CGEventSetFlags(e, kCGEventFlagMaskCommand);
		events.push_back(e);

		events.push_back(CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_Command, false));

		sendEvents(events);
	}

	void sendEscKeystroke()
	{
		vector<CGEventRef> events;
		events.push_back(CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_Escape, true));
		events.push_back(CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_Escape, false));
		sendEvents(events);
	}

	void sendScreenshotKeystroke()
	{
		vector<CGEventRef> events;

		events.push_back(CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_Command, true));
		events.push_back(CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_Shift,   true));
		CGEventRef e = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_ANSI_4,  true);
		CGEventSetFlags(e, kCGEventFlagMaskCommand ^ kCGEventFlagMaskShift);
		events.push_back(e);

		e = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_ANSI_4,  false);
		CGEventSetFlags(e, kCGEventFlagMaskCommand ^ kCGEventFlagMaskShift);
		events.push_back(e);
		events.push_back(CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_Command, false));
		events.push_back(CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_Shift,   false));

		sendEvents(events);
	}

	void sendMouseMove(int x, int y)
	{
		vector<CGEventRef> events;
		events.push_back(CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, CGPointMake(x,y), kCGMouseButtonLeft));
		sendEvents(events);
	}

	void sendMouseMoveWithDelta(int x, int y, int dx, int dy)
	{
		vector<CGEventRef> events;

		CGEventRef e = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, CGPointMake(x,y), kCGMouseButtonLeft);
		CGEventSetIntegerValueField(e, kCGMouseEventDeltaX, dx);
		CGEventSetIntegerValueField(e, kCGMouseEventDeltaY, dy);
		events.push_back(e);

		sendEvents(events);
	}

	void sendLeftMouseButton(int x, int y, bool buttonDown)
	{
		vector<CGEventRef> events;
		events.push_back(CGEventCreateMouseEvent(NULL, buttonDown ? kCGEventLeftMouseDown : kCGEventLeftMouseUp, CGPointMake(x,y), kCGMouseButtonLeft));
		sendEvents(events);
	}

	void sendLeftMouseClick(int x, int y, int clickCount)
	{
		vector<CGEventRef> events;

		for (int i = 0; i < clickCount; ++i)
		{
			CGEventRef mouseDown = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, CGPointMake(x,y), kCGMouseButtonLeft);
			CGEventSetIntegerValueField(mouseDown, kCGMouseEventClickState, clickCount);
			events.push_back(mouseDown);

			CGEventRef mouseUp = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, CGPointMake(x,y), kCGMouseButtonLeft);
			CGEventSetIntegerValueField(mouseUp, kCGMouseEventClickState, clickCount);
			events.push_back(mouseUp);
		}

		sendEvents(events, USEC_PER_SEC/20);
	}

	void sendLeftMouseDrag(int x, int y)
	{
		vector<CGEventRef> events;
		events.push_back(CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDragged, CGPointMake(x,y), 0));
		sendEvents(events);
	}

	// These compositions start out with a 1024x768 window on the primary screen,
	// with its content area at (200,100).
	const double windowX             = 200;
	const double windowY             = 100;
	const double windowContentWidth  = 1024;
	const double windowContentHeight = 768;
	const double windowTitlebarHeight= 22;
	VuoScreen screen;

private slots:

	void initTestCase()
	{
		screen = VuoScreen_getPrimary();
	}

	void testMouseEventsInWindow()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputMousePositionAndButton.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();
		VuoRunner::Port *movedToPort  = runner->getPublishedOutputPortWithName("MovedTo");
		VuoRunner::Port *pressedPort  = runner->getPublishedOutputPortWithName("Pressed");
		VuoRunner::Port *releasedPort = runner->getPublishedOutputPortWithName("Released");

		// When the composition starts, showedWindow fires, causing `Receive Mouse Moves` to fire its initial position.
		EXPECT_ANY_EVENT(^{
			VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		});
		// Then, the composition moves its window to a known position.
		// It fires showedWindow again, but since it's the same window, `Receive Mouse Moves` doesn't fire again.

		// Wait for the window to gain focus.
		sleep(4);

		{
			// Move the mouse to the top left of the window,
			// and make sure the Vuo mouse coordinates are (-1, +1/aspect).
			VuoPoint2d expectedPoint = (VuoPoint2d){ -1, windowContentHeight/windowContentWidth };
			EXPECT_EVENT(movedToPort, expectedPoint, ^{
				sendMouseMove(windowX, windowY);
			});

			// https://b33p.net/kosada/node/12339
			// Ensure that pressing the mouse in the resize area does not register as a mouse press.
			EXPECT_NO_EVENT(^{
				sendLeftMouseButton(windowX, windowY, true);
			});
			EXPECT_NO_EVENT(^{
				sendLeftMouseButton(windowX, windowY, false);
			});

			// Move the mouse further inside the window (past the resize area).
			// Press and release the button.
			expectedPoint = (VuoPoint2d){ -1 + 15*2/windowContentWidth, windowContentHeight/windowContentWidth - 15*2/windowContentWidth };
			EXPECT_EVENT(pressedPort, expectedPoint, ^{
				sendLeftMouseButton(windowX + 15, windowY + 15, true);
			});

			EXPECT_EVENT(releasedPort, expectedPoint, ^{
				sendLeftMouseButton(windowX + 15, windowY + 15, false);
			});
		}

		{
			// Move the mouse to the bottom right of the window,
			// and make sure Vuo outputs coordinate (+1, -1/aspect).
			double mouseX = windowX + windowContentWidth-1;
			double mouseY = windowY + windowContentHeight-1;
			VuoPoint2d expectedPoint = (VuoPoint2d){ 1, -windowContentHeight/windowContentWidth };
			EXPECT_EVENT(movedToPort, expectedPoint, ^{
				sendMouseMove(mouseX, mouseY);
			});

			// Make sure the button press and release are not reported (since this is a resize area).
			expectedPoint = (VuoPoint2d){ 1 - 15*2/windowContentWidth, -windowContentHeight/windowContentWidth + 15*2/windowContentWidth };
			EXPECT_EVENT(pressedPort, expectedPoint, ^{
				sendLeftMouseButton(mouseX - 15, mouseY - 15, true);
			});

			EXPECT_EVENT(releasedPort, expectedPoint, ^{
				sendLeftMouseButton(mouseX - 15, mouseY - 15, false);
			});
		}

		{
			// Move the mouse to the window's titlebar.
			double titlebarX = windowX + windowContentWidth/2;
			double titlebarY = windowY - windowTitlebarHeight/2;
			VuoPoint2d expectedPoint = (VuoPoint2d){ 1/windowContentWidth, (windowContentHeight/2 + windowTitlebarHeight/2)*2/windowContentWidth };
			EXPECT_EVENT(movedToPort, expectedPoint, ^{
				sendMouseMove(titlebarX, titlebarY);
			});

			// https://b33p.net/kosada/node/8934
			// Press the mouse button — there shouldn't be an event since the mouse is outside the window's content area.
			EXPECT_NO_EVENT(^{
				sendLeftMouseButton(titlebarX, titlebarY, true);
			});

			// Release the mouse button — there shouldn't be an event since the mouse is outside the window's content area.
			EXPECT_NO_EVENT(^{
				sendLeftMouseButton(titlebarX, titlebarY, false);
			});
		}

		runner->stop();
		delete runner;
	}

	void testMouseEventsFullscreen()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputMousePositionAndButton.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();
		VuoRunner::Port *movedToPort  = runner->getPublishedOutputPortWithName("MovedTo");
		VuoRunner::Port *pressedPort  = runner->getPublishedOutputPortWithName("Pressed");
		VuoRunner::Port *releasedPort = runner->getPublishedOutputPortWithName("Released");

		// When the composition starts, showedWindow fires, causing `Receive Mouse Moves` to fire its initial position.
		EXPECT_ANY_EVENT(^{
			sleep(2);
			VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		});
		// Then, the composition moves its window to a known position.
		// It fires showedWindow again, but since it's the same window, `Receive Mouse Moves` doesn't fire again.

		// Wait for the window to gain focus.
		sleep(2);

		// Make the composition's window fullscreen.
		EXPECT_NO_EVENT(^{
			sendFullscreenKeystroke();
		});

		// Give the fullscreen transition time to complete.
		sleep(2);

		{
			// Move the mouse to the full-left, near-top of the screen,
			// and make sure Vuo outputs coordinate (-1, slightly less than +1/aspect).
			// (Can't move all the way to the top, since it reveals the menu bar.)
			VuoPoint2d expectedPoint = (VuoPoint2d){ -1, (double)screen.height/screen.width - 10*2/(double)screen.width };
			EXPECT_EVENT(movedToPort, expectedPoint, ^{
				sendMouseMove(0, 10);
			});

			// https://b33p.net/kosada/node/8489
			// Make sure the button press and release are reported
			// (confirm that this point is considered within the screen).
			EXPECT_EVENT(pressedPort, expectedPoint, ^{
				sendLeftMouseButton(0, 10, true);
			});

			EXPECT_EVENT(releasedPort, expectedPoint, ^{
				sendLeftMouseButton(0, 10, false);
			});
		}

		{
			// https://b33p.net/kosada/node/11856
			// Move the mouse to the bottom right of the screen,
			// and make sure Vuo outputs coordinate (+1, -1/aspect).
			// The mouse is intentionally moved to the screen's width and height (rather than width-1 and height-1),
			// to account for the Cocoa bug where it returns mouse coordinates with an extra row and column of pixels.
			VuoPoint2d expectedPoint = (VuoPoint2d){ 1, -(double)screen.height/screen.width };
			EXPECT_EVENT(movedToPort, expectedPoint, ^{
				sendMouseMove(screen.width, screen.height);
			});

			// Make sure the button press and release are reported
			// (confirm that this point is considered within the screen).
			// This test doesn't work since [NSEvent mouseLocation] returns bogus mouse coordinates
			// after simulating a button-down in the bottom right corner of the screen,
			// maybe because it thinks the coordinates are out of bounds.
//			EXPECT_EVENT(pressedPort, expectedPoint, ^{
//				sendLeftMouseButton(screen.width, screen.height, true);
//			});

//			EXPECT_EVENT(releasedPort, expectedPoint, ^{
//				sendLeftMouseButton(screen.width, screen.height, false);
//			});
		}

		runner->stop();
		delete runner;
	}

	void testMouseEventsFullscreenAspectLocked()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputMousePositionAndButton-AspectLocked.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();
		VuoRunner::Port *movedToPort  = runner->getPublishedOutputPortWithName("MovedTo");
		VuoRunner::Port *pressedPort  = runner->getPublishedOutputPortWithName("Pressed");
		VuoRunner::Port *releasedPort = runner->getPublishedOutputPortWithName("Released");

		// When the composition starts, showedWindow fires, causing `Receive Mouse Moves` to fire its initial position.
		EXPECT_ANY_EVENT(^{
			sleep(2);
			VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		});
		// Then, the composition moves its window to a known position, resizes it, and changes its aspect ratio.
		// It fires showedWindow again, but since it's the same window, `Receive Mouse Moves` doesn't fire again.

		// Wait for the window to gain focus.
		sleep(2);

		// Make the composition's window fullscreen.
		EXPECT_NO_EVENT(^{
			sendFullscreenKeystroke();
		});

		// Give the fullscreen transition time to complete.
		sleep(2);

		// The window's aspect is 1:1, and the screen is assumed to have aspect > 1.
		// When fullscreen, the window should be centered on the screen.
		int windowWidth = screen.height;
		int windowLeftInScreenCoordinates = screen.width/2 - windowWidth/2;
		int windowRightInScreenCoordinates = screen.width/2 + windowWidth/2;

		{
			// Move the mouse to the full-left, near-top of the composition window,
			// and make sure Vuo outputs coordinate (-1, slightly less than +1/aspect).
			// (Can't move all the way to the top, since it reveals the menu bar.)

			VuoPoint2d expectedPoint = (VuoPoint2d){ -1, 1. - 10*2/(double)windowWidth };
			EXPECT_EVENT(movedToPort, expectedPoint, ^{
				sendMouseMove(windowLeftInScreenCoordinates, 10);
			});

			// https://b33p.net/kosada/node/8489
			// Make sure the button press and release are reported
			// (confirm that this point is considered within the screen).
			EXPECT_EVENT(pressedPort, expectedPoint, ^{
				sendLeftMouseButton(windowLeftInScreenCoordinates, 10, true);
			});

			EXPECT_EVENT(releasedPort, expectedPoint, ^{
				sendLeftMouseButton(windowLeftInScreenCoordinates, 10, false);
			});
		}

		{
			// https://b33p.net/kosada/node/11856
			// Move the mouse to the bottom right of the window,
			// and make sure Vuo outputs coordinate (+1, -1).
			// The mouse is intentionally moved to the screen's height (rather than height-1),
			// to account for the Cocoa bug where it returns mouse coordinates with an extra row and column of pixels.
			VuoPoint2d expectedPoint = (VuoPoint2d){ 1, -1 };
			EXPECT_EVENT(movedToPort, expectedPoint, ^{
				sendMouseMove(windowRightInScreenCoordinates, screen.height);
			});
		}

		runner->stop();
		delete runner;
	}

	void testMouseEventsFullscreenSizeLocked()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputMousePositionAndButton-SizeLocked.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();
		VuoRunner::Port *movedToPort  = runner->getPublishedOutputPortWithName("MovedTo");
		VuoRunner::Port *pressedPort  = runner->getPublishedOutputPortWithName("Pressed");
		VuoRunner::Port *releasedPort = runner->getPublishedOutputPortWithName("Released");

		// When the composition starts, showedWindow fires, causing `Receive Mouse Moves` to fire its initial position.
		EXPECT_ANY_EVENT(^{
			sleep(2);
			VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		});
		// Then, the composition moves its window to a known position, resizes it, and changes its aspect ratio.
		// It fires showedWindow again, but since it's the same window, `Receive Mouse Moves` doesn't fire again.

		// Wait for the window to gain focus.
		sleep(2);

		// Make the composition's window fullscreen.
		EXPECT_NO_EVENT(^{
			sendFullscreenKeystroke();
		});

		// Give the fullscreen transition time to complete.
		sleep(2);

		// When fullscreen, the window should be centered on the screen.
		int windowWidth = 1024;
		int windowHeight = 768;
		int windowLeftInScreenCoordinates = screen.width/2 - windowWidth/2;
		int windowRightInScreenCoordinates = screen.width/2 + windowWidth/2;
		int windowTopInScreenCoordinates = screen.height/2 - windowHeight/2;
		int windowBottomInScreenCoordinates = screen.height/2 + windowHeight/2;

		{
			// Move the mouse to the top-left of the composition window,
			// and make sure Vuo outputs coordinate (-1, +1/aspect).

			VuoPoint2d expectedPoint = (VuoPoint2d){ -1, (double)windowHeight/windowWidth };
			EXPECT_EVENT(movedToPort, expectedPoint, ^{
				sendMouseMove(windowLeftInScreenCoordinates, windowTopInScreenCoordinates);
			});

			// https://b33p.net/kosada/node/8489
			// Make sure the button press and release are reported
			// (confirm that this point is considered within the screen).
			EXPECT_EVENT(pressedPort, expectedPoint, ^{
				sendLeftMouseButton(windowLeftInScreenCoordinates, windowTopInScreenCoordinates, true);
			});

			EXPECT_EVENT(releasedPort, expectedPoint, ^{
				sendLeftMouseButton(windowLeftInScreenCoordinates, windowTopInScreenCoordinates, false);
			});
		}

		{
			// https://b33p.net/kosada/node/11856
			// Move the mouse to the bottom right of the window,
			// and make sure Vuo outputs coordinate (+1, -1/aspect).
			VuoPoint2d expectedPoint = (VuoPoint2d){ 1, -(double)windowHeight/windowWidth };
			EXPECT_EVENT(movedToPort, expectedPoint, ^{
				sendMouseMove(windowRightInScreenCoordinates, windowBottomInScreenCoordinates);
			});
		}

		runner->stop();
		delete runner;
	}

	void testMouseEventsAfterTransitionFromFullscreenToWindow()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputMousePositionAndButton.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();
		VuoRunner::Port *movedToPort  = runner->getPublishedOutputPortWithName("MovedTo");
		VuoRunner::Port *pressedPort  = runner->getPublishedOutputPortWithName("Pressed");
		VuoRunner::Port *releasedPort = runner->getPublishedOutputPortWithName("Released");

		// When the composition starts, showedWindow fires, causing `Receive Mouse Moves` to fire its initial position.
		EXPECT_ANY_EVENT(^{
			sleep(2);
			VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		});
		// Then, the composition moves its window to a known position.
		// It fires showedWindow again, but since it's the same window, `Receive Mouse Moves` doesn't fire again.

		// Wait for the window to gain focus.
		sleep(2);

		// Make the composition's window fullscreen.
		EXPECT_NO_EVENT(^{
			sendFullscreenKeystroke();
		});

		// Give the fullscreen transition time to complete.
		sleep(2);

		// Switch back from fullscreen to windowed.
		EXPECT_NO_EVENT(^{
			sendFullscreenKeystroke();
		});

		// Give the fullscreen transition time to complete.
		sleep(2);

		{
			// https://b33p.net/kosada/node/6258
			// https://b33p.net/kosada/node/7545
			// Ensure mouse events are received after transitioning from fullscreen to a window.
			VuoPoint2d expectedPoint = (VuoPoint2d){ -1 + 15*2/windowContentWidth, windowContentHeight/windowContentWidth - 15*2/windowContentWidth };
			EXPECT_EVENT(movedToPort, expectedPoint, ^{
				sendMouseMove(windowX + 15, windowY + 15);
			});

			EXPECT_EVENT(pressedPort, expectedPoint, ^{
				sendLeftMouseButton(windowX + 15, windowY + 15, true);
			});

			EXPECT_EVENT(releasedPort, expectedPoint, ^{
				sendLeftMouseButton(windowX + 15, windowY + 15, false);
			});
		}

		{
			// https://b33p.net/kosada/node/7545
			// Ensure mouse move events outside the window are received when the window is focused
			// after transitioning from fullscreen to a window.
			VuoPoint2d expectedPoint = (VuoPoint2d){ -1 - 10*2/windowContentWidth, windowContentHeight/windowContentWidth };
			EXPECT_EVENT(movedToPort, expectedPoint, ^{
							 sendMouseMove(windowX - 10, windowY);
						 });

			expectedPoint = (VuoPoint2d){ -1 - 20*2/windowContentWidth, windowContentHeight/windowContentWidth };
			EXPECT_EVENT(movedToPort, expectedPoint, ^{
							 sendMouseMove(windowX - 20, windowY);
						 });
		}

		runner->stop();
		delete runner;
	}

	void testClickthrough()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputMousePositionAndButton.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();
		VuoRunner::Port *pressedPort  = runner->getPublishedOutputPortWithName("Pressed");
		VuoRunner::Port *releasedPort = runner->getPublishedOutputPortWithName("Released");

		// When the composition starts, showedWindow fires, causing `Receive Mouse Moves` to fire its initial position.
		EXPECT_ANY_EVENT(^{
			sleep(2);
			VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		});
		// Then, the composition moves its window to a known position.
		// It fires showedWindow again, but since it's the same window, `Receive Mouse Moves` doesn't fire again.

		// Wait for the window to gain focus.
		sleep(3);

		{
			// Click outside the window to defocus it.
			EXPECT_NO_EVENT(^{
				sendLeftMouseClick(windowX - 20, windowY, 1);
			});

			EXPECT_NO_EVENT(^{
				sendMouseMove(windowX, windowY);
			});

			// https://vuo.org/node/1267#comment-2550
			// The first click inside the window's content area should fire an event.
			VuoPoint2d expectedPoint = (VuoPoint2d){ -1 + 15*2/windowContentWidth, windowContentHeight/windowContentWidth - 15*2/windowContentWidth };
			EXPECT_EVENT(pressedPort, expectedPoint, ^{
				sendLeftMouseButton(windowX + 15, windowY + 15, true);
			});
			EXPECT_EVENT(releasedPort, expectedPoint, ^{
				sendLeftMouseButton(windowX + 15, windowY + 15, false);
			});

			// Click outside the window to defocus it.
			sendLeftMouseClick(windowX - 20, windowY, 1);

			// Clicking on the window's titlebar should activate the window, but should not fire a moved, pressed, or released event.
			double titlebarX = windowX + windowContentWidth/2;
			double titlebarY = windowY - windowTitlebarHeight/2;
			EXPECT_NO_EVENT(^{
				sendLeftMouseClick(titlebarX, titlebarY, 1);
			});
		}

		runner->stop();
		delete runner;
	}

	void testEscToTransitionFromFullscreenToWindow()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputMousePositionAndButton.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();
		VuoRunner::Port *movedToPort  = runner->getPublishedOutputPortWithName("MovedTo");

		// When the composition starts, showedWindow fires, causing `Receive Mouse Moves` to fire its initial position.
		EXPECT_ANY_EVENT(^{
			sleep(2);
			VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		});
		// Then, the composition moves its window to a known position.
		// It fires showedWindow again, but since it's the same window, `Receive Mouse Moves` doesn't fire again.

		// Wait for the window to gain focus.
		sleep(2);

		{
			// Make the composition's window fullscreen.
			sendFullscreenKeystroke();

			// Give the fullscreen transition time to complete.
			sleep(2);

			// Ensure the window is fullscreen.
			VuoPoint2d expectedPoint = (VuoPoint2d){ -1, (double)screen.height/screen.width };
			EXPECT_EVENT(movedToPort, expectedPoint, ^{
				sendMouseMove(0,0);
			});

			// https://b33p.net/kosada/node/7859
			// https://b33p.net/kosada/node/7866
			// Esc should switch back from fullscreen to windowed.
			sendEscKeystroke();

			// Give the fullscreen transition time to complete.
			sleep(2);

			// Ensure the window is not fullscreen.
			expectedPoint = (VuoPoint2d){ -1, windowContentHeight/windowContentWidth };
			EXPECT_EVENT(movedToPort, expectedPoint, ^{
				sendMouseMove(windowX, windowY);
			});
		}

		runner->stop();
		delete runner;
	}

	void testMouseEventsInTwoWindows()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputMousePosition-TwoWindows.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();
		VuoRunner::Port *listeningPort  = runner->getPublishedInputPortWithName("Listening");
		VuoRunner::Port *leftMovedPort  = runner->getPublishedOutputPortWithName("LeftMoved");
		VuoRunner::Port *rightMovedPort = runner->getPublishedOutputPortWithName("RightMoved");

		// When the composition starts, showedWindow fires, causing each `Receive Mouse Moves` to fire its initial position.
		// But Listening=false, so no event should be output.
		EXPECT_NO_EVENT(^{
			VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		});

		// Then, the composition moves its window to a known position.
		// It fires showedWindow again, but since it's the same window, `Receive Mouse Moves` doesn't fire again.

		// Wait for the window to gain focus.
		sleep(4);

		const int windowY = 256;
		const int leftWindowX = 256;
		const int rightWindowX = 512;
		const int width = 256;
		const int height = 256;
		json_object *t = json_object_new_boolean(true);

		// Focus the left window.
		EXPECT_NO_EVENT(^{
			sendLeftMouseClick(leftWindowX + width/2, windowY + height/2, 1);
		});

		// Start listening
		EXPECT_NO_EVENT(^{
			runner->setPublishedInputPortValue(listeningPort, t);
			runner->firePublishedInputPortEvent(listeningPort);
			sleep(1);
		});

		// Move the mouse to the top-left of the left window.
		// Only the left window (since it's focused) should fire an event.
		VuoPoint2d expectedPoint = (VuoPoint2d){ -1, 1 };
		EXPECT_EVENT(leftMovedPort, expectedPoint, ^{
			sendMouseMove(leftWindowX, windowY);
		});
		EXPECT_NO_EVENT(^{});

		// Move the mouse to the right window.
		// Only the left window (since it's still focused) should fire an event.
		VuoPoint2d expectedPoint2 = (VuoPoint2d){ 2. + 3./256, -1./256 };
		EXPECT_EVENT(leftMovedPort, expectedPoint2, ^{
			sendMouseMove(rightWindowX + width/2, windowY + height/2);
		});
		EXPECT_NO_EVENT(^{});

		// Focus the right window.
		EXPECT_NO_EVENT(^{
			sendLeftMouseClick(rightWindowX + width/2, windowY + height/2, 1);
		});

		// Move the mouse to the top-left of the right window.
		// Only the right window (since it's focused) should fire an event.
		VuoPoint2d expectedPoint3 = (VuoPoint2d){ -1, 1 };
		EXPECT_EVENT(rightMovedPort, expectedPoint3, ^{
			sendMouseMove(rightWindowX, windowY);
		});
		EXPECT_NO_EVENT(^{});

		runner->stop();
		delete runner;
	}

	void testMouseClicks()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputMouseClick.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();
		VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		VuoRunner::Port *singleClickedPort = runner->getPublishedOutputPortWithName("SingleClicked");
		VuoRunner::Port *doubleClickedPort = runner->getPublishedOutputPortWithName("DoubleClicked");
		VuoRunner::Port *tripleClickedPort = runner->getPublishedOutputPortWithName("TripleClicked");

		VuoPoint2d expectedWindowedPoint = (VuoPoint2d){ -1 + 10*2/windowContentWidth, windowContentHeight/windowContentWidth - 10*2/windowContentWidth };

		{
			// https://b33p.net/kosada/node/12339
			// Ensure that clicking the mouse in the resize area does not register as a mouse click.
			EXPECT_NO_EVENT(^{
				sendMouseMove(windowX, windowY);
			});
			EXPECT_NO_EVENT(^{
				sendLeftMouseClick(windowX, windowY, 1);
			});

			// Single-click inside the top left of the window,
			// and make sure the Vuo mouse coordinates are correct.
			EXPECT_EVENT(singleClickedPort, expectedWindowedPoint, ^{
				sendMouseMove(windowX + 10, windowY + 10);
				sendLeftMouseClick(windowX + 10, windowY + 10, 1);
			});

			// Single-click inside the top left of the window,
			// and move the mouse before the double-click timeout expires.
			// Ensure the Vuo mouse coordinates are where the single-click occured
			// (not where it moved to after the click).
			EXPECT_EVENT(singleClickedPort, expectedWindowedPoint, ^{
				sendMouseMove(windowX + 10, windowY + 10);
				sendLeftMouseClick(windowX + 10, windowY + 10, 1);
				sendMouseMove(windowX + 100, windowY + 100);
			});

			// Make the composition's window fullscreen.
			EXPECT_NO_EVENT(^{
				sendFullscreenKeystroke();
			});

			// Give the fullscreen transition time to complete.
			sleep(2);

			// Single-click on the full-left, near-top of the screen,
			// and make sure the Vuo mouse coordinates are (-1, slightly less than +1/aspect).
			// (Can't move all the way to the top, since it reveals the menu bar.)
			VuoPoint2d expectedFullscreenPoint = (VuoPoint2d){ -1, (double)screen.height/screen.width - 10*2/(double)screen.width };
			EXPECT_EVENT(singleClickedPort, expectedFullscreenPoint, ^{
				sendMouseMove(0, 10);
				sendLeftMouseClick(0, 10, 1);
			});

			// https://b33p.net/kosada/node/11005
			// Single-click on the top left of the screen,
			// and move the mouse before the double-click timeout expires.
			// Ensure the Vuo mouse coordinates are where the single-click occured
			// (not where it moved to after the click).
			EXPECT_EVENT(singleClickedPort, expectedFullscreenPoint, ^{
				sendMouseMove(0, 10);
				sendLeftMouseClick(0, 10, 1);
				sendMouseMove(100, 100);
			});

			// Switch back from fullscreen to windowed.
			EXPECT_NO_EVENT(^{
				sendFullscreenKeystroke();
			});

			// Give the fullscreen transition time to complete.
			sleep(2);
		}

		{
			EXPECT_NO_EVENT(^{
				sendMouseMove(windowX + 10, windowY + 10);
			});

			// Double-click inside the top left of the window,
			// and make sure the Vuo mouse coordinates are correct.
			EXPECT_EVENT(doubleClickedPort, expectedWindowedPoint, ^{
				sendLeftMouseClick(windowX + 10, windowY + 10, 2);
			});
		}

		{
			// Triple-click inside the top left of the window,
			// and make sure the Vuo mouse coordinates are correct.
			EXPECT_EVENT(tripleClickedPort, expectedWindowedPoint, ^{
				sendLeftMouseClick(windowX + 10, windowY + 10, 3);
			});
		}

		{
			// Make sure quadruple (and higher) clicks are reported as triple-clicks.
			EXPECT_EVENT(tripleClickedPort, expectedWindowedPoint, ^{
							 sendLeftMouseClick(windowX + 10, windowY + 10, 4);
						 });
			EXPECT_EVENT(tripleClickedPort, expectedWindowedPoint, ^{
							 sendLeftMouseClick(windowX + 10, windowY + 10, 5);
						 });
		}

		runner->stop();
		delete runner;
	}

	void testMouseDeltas()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputMouseDelta.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();
		VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		VuoRunner::Port *movedByPort = runner->getPublishedOutputPortWithName("MovedBy");

		{
			// Move the mouse to the top left of the window.
			sendMouseMove(windowX, windowY);

			// Move the mouse down/right one pixel.
			VuoPoint2d expectedPoint = (VuoPoint2d){ 1*2/windowContentWidth, -1*2/windowContentWidth };
			EXPECT_EVENT(movedByPort, expectedPoint, ^{
				sendMouseMoveWithDelta(windowX + 1, windowY + 1, 1, 1);
			});

			// Move the mouse right one pixel.
			expectedPoint = (VuoPoint2d){ 1*2/windowContentWidth, 0 };
			EXPECT_EVENT(movedByPort, expectedPoint, ^{
				sendMouseMoveWithDelta(windowX + 2, windowY + 1, 1, 0);
			});

			// Move the mouse down one pixel.
			expectedPoint = (VuoPoint2d){ 0, -1*2/windowContentWidth };
			EXPECT_EVENT(movedByPort, expectedPoint, ^{
				sendMouseMoveWithDelta(windowX + 2, windowY + 2, 0, 1);
			});

			// https://b33p.net/kosada/node/11320
			// Move the mouse without changing the coordinates — shouldn't fire a delta event.
			EXPECT_NO_EVENT(^{
				sendMouseMoveWithDelta(windowX + 2, windowY + 2, 0, 0);
			});
		}

		runner->stop();
		delete runner;
	}

	void testMouseDrags()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputMouseDrag.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();
		VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		VuoRunner::Port *dragStartedPort = runner->getPublishedOutputPortWithName("DragStarted");
		VuoRunner::Port *dragMovedToPort = runner->getPublishedOutputPortWithName("DragMovedTo");
		VuoRunner::Port *dragEndedPort   = runner->getPublishedOutputPortWithName("DragEnded");

		{
			// Move the mouse near the top left of the window
			// (exact top/left doesn't work due to the window's resize handle).
			sendMouseMove(windowX + 10, windowY + 10);

			// Start dragging.
			VuoPoint2d expectedPoint = (VuoPoint2d){ -1 + 10*2/windowContentWidth, windowContentHeight/windowContentWidth - 10*2/windowContentWidth };
			EXPECT_EVENT(dragStartedPort, expectedPoint, ^{
				sendLeftMouseButton(windowX + 10, windowY + 10, true);
				sendLeftMouseDrag  (windowX + 10, windowY + 10);
			});

			EXPECT_EVENT(dragMovedToPort, expectedPoint, ^{});

			// Drag the mouse down/right 10 pixels.
			expectedPoint = (VuoPoint2d){ -1 + 20*2/windowContentWidth, windowContentHeight/windowContentWidth - 20*2/windowContentWidth };
			EXPECT_EVENT(dragMovedToPort, expectedPoint, ^{
				sendLeftMouseDrag(windowX + 20, windowY + 20);
			});

			// Stop dragging.
			EXPECT_EVENT(dragEndedPort, expectedPoint, ^{
				sendLeftMouseButton(windowX + 20, windowY + 20, false);
			});

			// https://b33p.net/kosada/node/11418
			// Start dragging inside the window, drag outside, then release.
			EXPECT_EVENT(dragStartedPort, expectedPoint, ^{
				sendLeftMouseButton(windowX + 20, windowY + 20, true);
				sendLeftMouseDrag  (windowX + 20, windowY + 20);
			});

			EXPECT_EVENT(dragMovedToPort, expectedPoint, ^{});

			// Drag the mouse up/left 40 pixels.
			expectedPoint = (VuoPoint2d){ -1 - 20*2/windowContentWidth, windowContentHeight/windowContentWidth + 20*2/windowContentWidth };
			EXPECT_EVENT(dragMovedToPort, expectedPoint, ^{
				sendLeftMouseDrag(windowX - 20, windowY - 20);
			});

			// Stop dragging.
			EXPECT_EVENT(dragEndedPort, expectedPoint, ^{
				sendLeftMouseButton(windowX - 20, windowY - 20, false);
			});
		}

		runner->stop();
		delete runner;
	}

	void testMouseStatusInWindow()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputMouseStatus.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();
		sleep(2);
		VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		VuoRunner::Port *whichPort     = runner->getPublishedInputPortWithName("Which");
		VuoRunner::Port *positionPort  = runner->getPublishedOutputPortWithName("Position");
		VuoRunner::Port *isPressedPort = runner->getPublishedOutputPortWithName("IsPressed");
		json_object *position  = json_object_new_int(1);
		json_object *isPressed = json_object_new_int(2);

		// Wait for the window to gain focus.
		sleep(2);

		{
			// Move the mouse to the top left of the window and check its coordinates.
			VuoPoint2d expectedPoint = (VuoPoint2d){ -1, windowContentHeight/windowContentWidth };
			EXPECT_EVENT(positionPort, expectedPoint, ^{
				sendMouseMove(windowX, windowY);
				runner->setPublishedInputPortValue(whichPort, position);
				runner->firePublishedInputPortEvent(whichPort);
			});
			EXPECT_EVENT(isPressedPort, false, ^{
				runner->setPublishedInputPortValue(whichPort, isPressed);
				runner->firePublishedInputPortEvent(whichPort);
			});

			// https://b33p.net/kosada/node/11580
			// Ensure that pressing the mouse in the resize area does not register as a mouse press.
			EXPECT_EVENT(isPressedPort, false, ^{
				sendLeftMouseButton(windowX, windowY, true);
				runner->setPublishedInputPortValue(whichPort, isPressed);
				runner->firePublishedInputPortEvent(whichPort);
			});
			EXPECT_EVENT(isPressedPort, false, ^{
				sendLeftMouseButton(windowX, windowY, false);
				runner->setPublishedInputPortValue(whichPort, isPressed);
				runner->firePublishedInputPortEvent(whichPort);
			});

			// Move the mouse further inside the window (past the resize area).
			// Press and release the button.
			EXPECT_EVENT(isPressedPort, true, ^{
				sendLeftMouseButton(windowX + 15, windowY + 15, true);
				runner->setPublishedInputPortValue(whichPort, isPressed);
				runner->firePublishedInputPortEvent(whichPort);
			});
			EXPECT_EVENT(isPressedPort, false, ^{
				sendLeftMouseButton(windowX + 15, windowY + 15, false);
				runner->setPublishedInputPortValue(whichPort, isPressed);
				runner->firePublishedInputPortEvent(whichPort);
			});
		}

		{
			// Move the mouse to the bottom right of the window and check its coordinates.
			double mouseX = windowX + windowContentWidth-1;
			double mouseY = windowY + windowContentHeight-1;
			VuoPoint2d expectedPoint = (VuoPoint2d){ 1, -windowContentHeight/windowContentWidth };
			EXPECT_EVENT(positionPort, expectedPoint, ^{
				sendMouseMove(mouseX, mouseY);
				runner->setPublishedInputPortValue(whichPort, position);
				runner->firePublishedInputPortEvent(whichPort);
			});
			EXPECT_EVENT(isPressedPort, false, ^{
				runner->setPublishedInputPortValue(whichPort, isPressed);
				runner->firePublishedInputPortEvent(whichPort);
			});

			// Press and release the button.
			// It shouldn't register a mouse click since clicks in this area are for resizing the window.
			EXPECT_EVENT(isPressedPort, false, ^{
				sendLeftMouseButton(mouseX, mouseY, true);
				runner->setPublishedInputPortValue(whichPort, isPressed);
				runner->firePublishedInputPortEvent(whichPort);
			});
			EXPECT_EVENT(isPressedPort, false, ^{
				sendLeftMouseButton(mouseX, mouseY, false);
				runner->setPublishedInputPortValue(whichPort, isPressed);
				runner->firePublishedInputPortEvent(whichPort);
			});

			// Move the mouse further inside the window (past the resize area).
			// Press and release the button.
			mouseX -= 15;
			mouseY -= 15;
			EXPECT_EVENT(isPressedPort, true, ^{
				sendLeftMouseButton(mouseX, mouseY, true);
				runner->setPublishedInputPortValue(whichPort, isPressed);
				runner->firePublishedInputPortEvent(whichPort);
			});
			EXPECT_EVENT(isPressedPort, false, ^{
				sendLeftMouseButton(mouseX, mouseY, false);
				runner->setPublishedInputPortValue(whichPort, isPressed);
				runner->firePublishedInputPortEvent(whichPort);
			});
		}

		{
			// Move the mouse to the window's titlebar.
			double titlebarX = windowX + windowContentWidth/2;
			double titlebarY = windowY - windowTitlebarHeight/2;
			VuoPoint2d expectedPoint = (VuoPoint2d){ 1/windowContentWidth, (windowContentHeight/2 + windowTitlebarHeight/2)*2/windowContentWidth };
			EXPECT_EVENT(positionPort, expectedPoint, ^{
				sendMouseMove(titlebarX, titlebarY);
				runner->setPublishedInputPortValue(whichPort, position);
				runner->firePublishedInputPortEvent(whichPort);
			});

			// https://b33p.net/kosada/node/8934
			// Press the mouse button — it shouldn't be reported as pressed since the mouse is outside the window's content area.
			EXPECT_EVENT(isPressedPort, false, ^{
				sendLeftMouseButton(titlebarX, titlebarY, true);
				runner->setPublishedInputPortValue(whichPort, isPressed);
				runner->firePublishedInputPortEvent(whichPort);
			});
			EXPECT_EVENT(isPressedPort, false, ^{
				sendLeftMouseButton(titlebarX, titlebarY, false);
				runner->setPublishedInputPortValue(whichPort, isPressed);
				runner->firePublishedInputPortEvent(whichPort);
			});

			// https://b33p.net/kosada/node/11651
			// If we press the button inside the content area, then release the button outside the content area,
			// the release should be reported.
			EXPECT_EVENT(isPressedPort, true, ^{
				sendLeftMouseButton(titlebarX, windowY, true);
				runner->setPublishedInputPortValue(whichPort, isPressed);
				runner->firePublishedInputPortEvent(whichPort);
			});
			EXPECT_EVENT(isPressedPort, false, ^{
				sendLeftMouseButton(titlebarX, titlebarY, false);
				runner->setPublishedInputPortValue(whichPort, isPressed);
				runner->firePublishedInputPortEvent(whichPort);
			});
		}

		runner->stop();
		delete runner;
	}

	void testMouseStatusFullscreen()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputMouseStatus.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();
		sleep(2);
		VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		VuoRunner::Port *whichPort     = runner->getPublishedInputPortWithName("Which");
		VuoRunner::Port *positionPort  = runner->getPublishedOutputPortWithName("Position");
		json_object *position  = json_object_new_int(1);

		// Wait for the window to gain focus.
		sleep(2);

		// Make the composition's window fullscreen.
		sendFullscreenKeystroke();

		// Give the fullscreen transition time to complete.
		sleep(2);

		{
			// Move the mouse to the top left of the screen,
			// and make sure Vuo outputs coordinate (-1, +1/aspect).
			VuoPoint2d expectedPoint = (VuoPoint2d){ -1, (double)screen.height/screen.width };
			EXPECT_EVENT(positionPort, expectedPoint, ^{
				sendMouseMove(0,0);
				runner->setPublishedInputPortValue(whichPort, position);
				runner->firePublishedInputPortEvent(whichPort);
			});
		}

		{
			// https://b33p.net/kosada/node/9799
			// Move the mouse to the bottom right of the screen,
			// and make sure Vuo outputs coordinate (+1, -1/aspect).
			// The mouse is intentionally moved to the screen's width and height (rather than width-1 and height-1),
			// to account for the Cocoa bug where it returns mouse coordinates with an extra row and column of pixels.
			VuoPoint2d expectedPoint = (VuoPoint2d){ 1, -(double)screen.height/screen.width };
			EXPECT_EVENT(positionPort, expectedPoint, ^{
				sendMouseMove(screen.width, screen.height);
				runner->setPublishedInputPortValue(whichPort, position);
				runner->firePublishedInputPortEvent(whichPort);
			});
		}

		runner->stop();
		delete runner;
	}

	void testMouseStatusFocus()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputMouseStatus.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();
		sleep(2);
		VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		VuoRunner::Port *whichPort     = runner->getPublishedInputPortWithName("Which");
		VuoRunner::Port *positionPort  = runner->getPublishedOutputPortWithName("Position");
		VuoRunner::Port *isPressedPort = runner->getPublishedOutputPortWithName("IsPressed");
		json_object *position  = json_object_new_int(1);
		json_object *isPressed = json_object_new_int(2);

		// Wait for the window to gain focus.
		sleep(2);

		{
			// Move the mouse outside the window and check its coordinates.
			// `Check Mouse Status` should still update its position when the mouse is outside the window.
			VuoPoint2d expectedPoint = (VuoPoint2d){ -1 - 20*2/windowContentWidth, windowContentHeight/windowContentWidth + 20*2/windowContentWidth };
			EXPECT_EVENT(positionPort, expectedPoint, ^{
				sendMouseMove(windowX - 20, windowY - 20);
				runner->setPublishedInputPortValue(whichPort, position);
				runner->firePublishedInputPortEvent(whichPort);
			});

			// https://b33p.net/kosada/node/7707
			// Press the button.
			// This should defocus the window (and the app).
			// `Check Mouse Status` should not report that the button is pressed, since it was pressed outside the window.
			EXPECT_EVENT(isPressedPort, false, ^{
				sendLeftMouseButton(windowX - 20, windowY - 20, true);
				runner->setPublishedInputPortValue(whichPort, isPressed);
				runner->firePublishedInputPortEvent(whichPort);
			});
			sendLeftMouseButton(windowX - 20, windowY - 20, false);

		}

		runner->stop();
		delete runner;
	}

	void testMouseStatusDuringScreenshot()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputMouseStatus.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();
		sleep(2);
		VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		VuoRunner::Port *whichPort     = runner->getPublishedInputPortWithName("Which");
		VuoRunner::Port *positionPort  = runner->getPublishedOutputPortWithName("Position");
		VuoRunner::Port *isPressedPort = runner->getPublishedOutputPortWithName("IsPressed");
		json_object *position  = json_object_new_int(1);
		json_object *isPressed = json_object_new_int(2);

		// Wait for the window to gain focus.
		sleep(2);

		{
			// Move the mouse to a known location.
			VuoPoint2d expectedPoint = (VuoPoint2d){ -1 + 10*2/windowContentWidth, windowContentHeight/windowContentWidth - 10*2/windowContentWidth };
			sendMouseMove(windowX + 10, windowY + 10);

			// Switch into screenshot mode.
			sendScreenshotKeystroke();

			// Mouse activity while in screenshot mode should not affect the position/isPressed reported by 'Check Mouse Status'.
			EXPECT_EVENT(isPressedPort, false, ^{
				sendLeftMouseButton(windowX + 10, windowY + 10, true);
				runner->setPublishedInputPortValue(whichPort, isPressed);
				runner->firePublishedInputPortEvent(whichPort);
			});
			EXPECT_EVENT(positionPort, expectedPoint, ^{
				sendLeftMouseDrag(windowX, windowY);
				sendLeftMouseDrag(windowX + 100, windowY + 100);
				runner->setPublishedInputPortValue(whichPort, position);
				runner->firePublishedInputPortEvent(whichPort);
			});

			// Cancel the screenshot so we don't clutter the desktop.
			sendEscKeystroke();
		}

		runner->stop();
		delete runner;
	}

	void testShowedWindow()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputShowedWindow.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();

		// When the composition starts, showedWindow fires.
		EXPECT_ANY_EVENT(^{
			VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		});

		// After that it shouldn't fire.
		EXPECT_NO_EVENT(^{});

		VuoRunner::Port *whichPort = runner->getPublishedInputPortWithName("Which");
		json_object *one = json_object_new_int(1);
		json_object *three = json_object_new_int(3);

		// An event to the Select node's first output should move the window, causing showedWindow to fire.
		EXPECT_ANY_EVENT(^{
			runner->setPublishedInputPortValue(whichPort, one);
			runner->firePublishedInputPortEvent(whichPort);
		});
		EXPECT_NO_EVENT(^{});

		// An event to the Select node's third output should re-render the window, but should not cause showedWindow to fire.
		EXPECT_NO_EVENT(^{
			runner->setPublishedInputPortValue(whichPort, three);
			runner->firePublishedInputPortEvent(whichPort);
		});

		// Make the composition's window fullscreen.
		// This should fire one showedWindow event, since the window changed position/size.
		EXPECT_ANY_EVENT(^{
			sendFullscreenKeystroke();
		});
		EXPECT_NO_EVENT(^{});

		// An event to the Select node's third output should re-render the window, but should not cause showedWindow to fire.
		EXPECT_NO_EVENT(^{
			runner->setPublishedInputPortValue(whichPort, three);
			runner->firePublishedInputPortEvent(whichPort);
		});

		// Switch back from fullscreen to windowed.
		// This should fire one showedWindow event, since the window changed position/size.
		EXPECT_ANY_EVENT(^{
			sendEscKeystroke();
		});
		EXPECT_NO_EVENT(^{});

		runner->stop();
		delete runner;
	}

#if 0
	void testShowedWindowAspectLocked()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputShowedWindow-AspectLocked.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();

		// When the composition starts, showedWindow fires.
		EXPECT_ANY_EVENT(^{
			VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		});

		// After that it shouldn't fire.
		EXPECT_NO_EVENT(^{});

		VuoRunner::Port *whichPort = runner->getPublishedInputPortWithName("Which");
		json_object *one = json_object_new_int(1);
		json_object *two = json_object_new_int(2);
		json_object *three = json_object_new_int(3);

		// An event to the Select node's first output should move the window, causing showedWindow to fire.
		EXPECT_ANY_EVENT(^{
			runner->setPublishedInputPortValue(whichPort, one);
			runner->firePublishedInputPortEvent(whichPort);
		});
		EXPECT_NO_EVENT(^{});

		// An event to the Select node's second output should lock the window's aspect ratio and resize the window, causing showedWindow to fire.
		EXPECT_ANY_EVENT(^{
			runner->setPublishedInputPortValue(whichPort, two);
			runner->firePublishedInputPortEvent(whichPort);
		});
		EXPECT_NO_EVENT(^{});

		// An event to the Select node's third output should re-render the window, but should not cause showedWindow to fire.
		EXPECT_NO_EVENT(^{
			runner->setPublishedInputPortValue(whichPort, three);
			runner->firePublishedInputPortEvent(whichPort);
		});

		// Make the composition's window fullscreen.
		// This should fire one showedWindow event, since the window changed position/size.
		EXPECT_ANY_EVENT(^{
			sendFullscreenKeystroke();
		});
		EXPECT_NO_EVENT(^{});

		// An event to the Select node's third output should re-render the window, but should not cause showedWindow to fire.
		EXPECT_NO_EVENT(^{
			runner->setPublishedInputPortValue(whichPort, three);
			runner->firePublishedInputPortEvent(whichPort);
		});

		// Switch back from fullscreen to windowed.
		// This should fire one showedWindow event, since the window changed position/size.
		EXPECT_ANY_EVENT(^{
			sendEscKeystroke();
		});
		EXPECT_NO_EVENT(^{});

		runner->stop();
		delete runner;
	}

	void testShowedWindowSizeLocked()
	{
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("compositions/OutputShowedWindow-SizeLocked.vuo");
		TestWindowDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();

		// When the composition starts, showedWindow fires.
		EXPECT_ANY_EVENT(^{
			VuoFileUtilities::focusProcess(runner->getCompositionPid(), true);
		});

		// After that it shouldn't fire.
		EXPECT_NO_EVENT(^{});

		VuoRunner::Port *whichPort = runner->getPublishedInputPortWithName("Which");
		json_object *one = json_object_new_int(1);
		json_object *two = json_object_new_int(2);
		json_object *three = json_object_new_int(3);

		// An event to the Select node's first output should move the window, causing showedWindow to fire.
		EXPECT_ANY_EVENT(^{
			runner->setPublishedInputPortValue(whichPort, one);
			runner->firePublishedInputPortEvent(whichPort);
		});
		EXPECT_NO_EVENT(^{});

		// An event to the Select node's second output should lock the window's aspect ratio,
		// but since it doesn't need to resize, it should not cause showedWindow to fire.
		EXPECT_NO_EVENT(^{
			runner->setPublishedInputPortValue(whichPort, two);
			runner->firePublishedInputPortEvent(whichPort);
		});

		// An event to the Select node's third output should re-render the window, but should not cause showedWindow to fire.
		EXPECT_NO_EVENT(^{
			runner->setPublishedInputPortValue(whichPort, three);
			runner->firePublishedInputPortEvent(whichPort);
		});

		// Make the composition's window fullscreen.
		// This should fire one showedWindow event, since the window changed position/size.
		EXPECT_ANY_EVENT(^{
			sendFullscreenKeystroke();
		});
		EXPECT_NO_EVENT(^{});

		// An event to the Select node's third output should re-render the window, but should not cause showedWindow to fire.
		EXPECT_NO_EVENT(^{
			runner->setPublishedInputPortValue(whichPort, three);
			runner->firePublishedInputPortEvent(whichPort);
		});

		// Switch back from fullscreen to windowed.
		// This should fire one showedWindow event, since the window changed position/size.
		EXPECT_ANY_EVENT(^{
			sendEscKeystroke();
		});
		EXPECT_NO_EVENT(^{});

		runner->stop();
		delete runner;
	}
#endif
};

QTEST_APPLESS_MAIN(TestWindow)
#include "TestWindow.moc"
