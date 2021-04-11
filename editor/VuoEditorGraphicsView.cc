/**
 * @file
 * VuoEditorGraphicsView implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoEditorGraphicsView.hh"
#include "VuoEditor.hh"
#include "VuoEditorCocoa.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"

/**
 * Creates a VuoEditorGraphicsView.
 */
VuoEditorGraphicsView::VuoEditorGraphicsView(QWidget *parent)
	: QGraphicsView(parent)
{
	setOptimizationFlags(QGraphicsView::DontSavePainterState | QGraphicsView::DontAdjustForAntialiasing);

	// For large compositions, it's faster to redraw the entire viewport
	// than to check hundreds of objects to determine whether we should render them.
	setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

	framesRenderedSinceProfileLogged = 0;
	renderDurationSinceProfileLogged = 0;
	lastProfileLoggedTime = VuoLogGetElapsedTime();

	viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
	accumulatedScale = 1;
	gestureDetected = false;

	QSettings settings;
	ignorePinchesSmallerThanX = settings.value("canvas/ignorePinchesSmallerThanX", 0.25).toDouble();
	ignorePinchesStartedLessThanXSecondsAfterDrag = settings.value("canvas/ignorePinchesStartedLessThanXSecondsAfterDrag", 0.5).toDouble();
	ignorePinchesStartedMoreThanXSecondsAfterTouch = settings.value("canvas/ignorePinchesStartedMoreThanXSecondsAfterTouch", 0.25).toDouble();

	VuoEditor *editor = (VuoEditor *)qApp;
	connect(editor, &VuoEditor::darkInterfaceToggled, this, &VuoEditorGraphicsView::updateColor);
}

/**
 * Returns true if a pinch zoom is currently being performed.
 */
bool VuoEditorGraphicsView::pinchZoomInProgress()
{
	return gestureDetected;
}

/**
 * Handles events for the VuoEditorGraphicsView, skimming the node library keyboard shortcut off the top.
 */
bool VuoEditorGraphicsView::event(QEvent * event)
{
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = (QKeyEvent *)(event);
		if (keyEvent->key() == Qt::Key_Return && (keyEvent->modifiers() & Qt::ControlModifier))
		{
			VuoEditor *e = (VuoEditor *)QCoreApplication::instance();
			e->showNodeLibrary();
			return true;
		}

		if (keyEvent->key() == Qt::Key_Escape)
			qApp->sendEvent(parentWidget(), keyEvent);
	}

	else if (event->type() == QEvent::Resize)
		emit viewResized();

	return QGraphicsView::event(event);
}

/**
 * Renders and logs statistics.
 */
void VuoEditorGraphicsView::paintEvent(QPaintEvent *event)
{
	double t0 = VuoLogGetElapsedTime();
	QGraphicsView::paintEvent(event);
	double renderDuration = VuoLogGetElapsedTime() - t0;

	if (VuoIsDebugEnabled())
	{
		++framesRenderedSinceProfileLogged;
		renderDurationSinceProfileLogged += renderDuration;
		const double profileSeconds = 5;
		if (t0 > lastProfileLoggedTime + profileSeconds)
		{
			VUserLog("%4d items, average draw time %6.4f s (%6.1f fps), %6.1f draws per second, %4d MB cached, bspDepth %2d",
					 scene()->items().count(),
					 renderDurationSinceProfileLogged/framesRenderedSinceProfileLogged,
					 framesRenderedSinceProfileLogged/renderDurationSinceProfileLogged,
					 framesRenderedSinceProfileLogged/profileSeconds,
					 QPixmapCache::totalUsed()/1024,
					 scene()->bspTreeDepth());
#if 0
			foreach (QGraphicsItem *i, scene()->items())
			{
				VuoRendererNode *node = dynamic_cast<VuoRendererNode *>(i);
				VuoRendererPort *port = dynamic_cast<VuoRendererPort *>(i);
				if (node)
					VUserLog("\t%p node '%s'", i, node->getBase()->getTitle().c_str());
				else if (port)
					VUserLog("\t%p port '%s'", i, port->getBase()->getClass()->getName().c_str());
				else
					VUserLog("\t%p %s", i, typeid(*i).name());
			}
#endif

			lastProfileLoggedTime = t0;
			framesRenderedSinceProfileLogged = 0;
			renderDurationSinceProfileLogged = 0;
		}
	}
}

/**
 * Handles pinch-zoom gestures.
 */
bool VuoEditorGraphicsView::viewportEvent(QEvent *event)
{
	QEvent::Type eventType = event->type();
	VuoEditorComposition *composition = static_cast<VuoEditorComposition *>(scene());


	// See VuoEditorWindow::eventFilter.
	// https://b33p.net/kosada/node/16688
	if (eventType == QEvent::MouseButtonPress)
	{
		auto mbp = static_cast<QMouseEvent *>(event);
		if (mbp->button() == Qt::RightButton)
		{
			QGraphicsSceneContextMenuEvent contextMenuEvent(QEvent::GraphicsSceneContextMenu);
			contextMenuEvent.setScreenPos(mbp->globalPos());
			contextMenuEvent.setScenePos(mapToScene(mbp->pos()));
			contextMenuEvent.setReason(QGraphicsSceneContextMenuEvent::Mouse);
			QApplication::sendEvent(scene(), &contextMenuEvent);
			event->accept();
			return true;
		}
	}


	if (eventType == QEvent::TouchBegin
	 || eventType == QEvent::TouchUpdate
	 || eventType == QEvent::TouchEnd)
	{
		QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
		const QList<QTouchEvent::TouchPoint> &touchPoints = touchEvent->touchPoints();
		const QTouchEvent::TouchPoint &touchPoint0 = touchPoints.first();
		const QTouchEvent::TouchPoint &touchPoint1 = touchPoints.last();

		static double latestTouchBeganTime = 0;
		if (touchPoint0.state() & Qt::TouchPointPressed
		 || touchPoint1.state() & Qt::TouchPointPressed)
			latestTouchBeganTime = ((double)touchEvent->timestamp()) / 1000.;

		if (static_cast<VuoEditorWindow *>(window())->isScrollInProgress())
		{
			VDebugLog("ignoring pinch while scroll in progress");
			return false;
		}

		double secondsSinceLastDrag = VuoLogGetElapsedTime() - static_cast<VuoEditorWindow *>(window())->getLatestDragTime();
		if (secondsSinceLastDrag < ignorePinchesStartedLessThanXSecondsAfterDrag)
		{
			VDebugLog("ignoring pinch that occurred just %.2fs (less than %gs) after the latest drag", secondsSinceLastDrag, ignorePinchesStartedLessThanXSecondsAfterDrag);
			return false;
		}

		if (static_cast<VuoEditorWindow *>(window())->isItemDragInProgress())
		{
			VDebugLog("ignoring pinch while item drag in progress");
			return false;
		}

		if (composition && composition->getCableInProgress())
		{
			VDebugLog("ignoring pinch while cable drag in progress");
			return false;
		}

		if (!rubberBandRect().isNull())
		{
			VDebugLog("ignoring pinch while rubberband in progress");
			return false;
		}

		if (touchPoints.count() == 2)
		{
			double currentLength = QLineF(touchPoint0.pos(), touchPoint1.pos()).length();

			static double detectedLength = 0;
			if (!gestureDetected)
			{
				// Ignore tiny gestures (which probably aren't intended to be pinch gestures anyway).
				double startLength   = QLineF(touchPoint0.startPos(), touchPoint1.startPos()).length();
				double scale = currentLength / startLength;
				if (fabs(scale - 1) < ignorePinchesSmallerThanX)
				{
					VDebugLog("ignoring small pinch (%.2f < %g)", fabs(scale-1), ignorePinchesSmallerThanX);
					return false;
				}

				// Ignore pinch gestures that start long after both fingers are down
				// (which probably aren't intended to be pinch gestures anyway).
				double secondsSinceTouch = ((double)touchEvent->timestamp())/1000. - latestTouchBeganTime;
				if (secondsSinceTouch > ignorePinchesStartedMoreThanXSecondsAfterTouch)
				{
					VDebugLog("ignoring pinch that started %.2fs (more than %gs) after touch", secondsSinceTouch, ignorePinchesStartedMoreThanXSecondsAfterTouch);
					return false;
				}

				detectedLength = QLineF(touchPoint0.pos(), touchPoint1.pos()).length();

				// At the start of each pinch gesture, use the view's current scale.
				accumulatedScale = transform().m11();

				setInteractive(false);
				gestureDetected = true;
			}

			double scale = currentLength / detectedLength;

			if (touchEvent->touchPointStates() & Qt::TouchPointReleased)
			{
				accumulatedScale *= scale;
				scale = 1;
				setInteractive(true);
				gestureDetected = false;
			}

			else
			{
				// Skip rendering if we aren't keeping up.
				double lag = VuoEditorCocoa_systemUptime() - touchEvent->timestamp()/1000.;
				const double lagLimit = .1;
				if (lag > lagLimit)
					return true;
			}

			VDebugLog("pinch zoomed to scale %g", accumulatedScale * scale);
			setTransform(QTransform::fromScale(accumulatedScale * scale, accumulatedScale * scale));

			// After zooming, scroll the scene to the view's center.
			centerOn(mapToScene(rect().center()));

			static_cast<VuoEditorWindow *>(window())->updateUI();
		}
		return true;
	}

	return QGraphicsView::viewportEvent(event);
}

/**
 * Makes the widget dark.
 */
void VuoEditorGraphicsView::updateColor(bool isDark)
{
	VuoEditorComposition *composition = static_cast<VuoEditorComposition *>(scene());
	composition->setColor(isDark);
}
