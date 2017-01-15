/**
 * @file
 * VuoRendererSignaler implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
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
 * Emits a @c nodeTitleEditorRequested signal.
 */
void VuoRendererSignaler::signalSubcompositionEditRequested(VuoRendererNode *node)
{
	emit subcompositionEditRequested(node);
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
