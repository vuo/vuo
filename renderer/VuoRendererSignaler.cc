/**
 * @file
 * VuoRendererSignaler implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRendererSignaler.hh"

/**
 * Creates a signaler.
 */
VuoRendererSignaler::VuoRendererSignaler(void) :
	QObject()
{
}

/**
 * Emits a @c nodesMoved signal.
 */
void VuoRendererSignaler::signalNodesMoved(set<VuoRendererNode *> nodes, qreal dx, qreal dy, bool movedByDragging)
{
	emit nodesMoved(nodes, dx, dy, movedByDragging);
}

/**
 * Emits a @c commentsMoved signal.
 */
void VuoRendererSignaler::signalCommentsMoved(set<VuoRendererComment *> comments, qreal dx, qreal dy, bool movedByDragging)
{
	emit commentsMoved(comments, dx, dy, movedByDragging);
}

/**
 * Emits a @c signalNodePopoverRequested signal.
 */
void VuoRendererSignaler::signalNodePopoverRequested(VuoRendererNode *node)
{
	emit nodePopoverRequested(node);
}

/**
 * Emits a @c inputEditorRequested signal.
 */
void VuoRendererSignaler::signalInputEditorRequested(VuoRendererPort *port)
{
	emit inputEditorRequested(port);
}

/**
 * Emits a @c nodeTitleEditorRequested signal.
 */
void VuoRendererSignaler::signalNodeTitleEditorRequested(VuoRendererNode *node)
{
	emit nodeTitleEditorRequested(node);
}

/**
 * Emits a @c commentEditorRequested signal.
 */
void VuoRendererSignaler::signalCommentEditorRequested(VuoRendererComment *comment)
{
	emit commentEditorRequested(comment);
}

/**
 * Emits a @c commentZoomRequested signal.
 */
void VuoRendererSignaler::signalCommentZoomRequested(VuoRendererComment *comment)
{
	emit commentZoomRequested(comment);
}

/**
 * Emits a @c commentResized signal.
 */
void VuoRendererSignaler::signalCommentResized(VuoRendererComment *comment, qreal dx, qreal dy)
{
	emit commentResized(comment, dx, dy);
}

/**
 * Emits a @c nodeSourceEditorRequested signal.
 */
void VuoRendererSignaler::signalNodeSourceEditorRequested(VuoRendererNode *node)
{
	emit nodeSourceEditorRequested(node);
}

/**
 * Emits a @c inputPortCountAdjustmentRequested signal.
 */
void VuoRendererSignaler::signalInputPortCountAdjustmentRequested(VuoRendererNode *node, int inputPortCountDelta, bool requestedByDragging)
{
	emit inputPortCountAdjustmentRequested(node, inputPortCountDelta, requestedByDragging);
}

/**
* Emits a @c dragStickinessDisabled signal.
*/
void VuoRendererSignaler::signalDisableDragStickiness(bool disable)
{
	emit dragStickinessDisableRequested(disable);
}

/**
 * Emits an `openUrlRequested` signal.
 */
void VuoRendererSignaler::signalOpenUrl(QString url)
{
	emit openUrl(url);
}
