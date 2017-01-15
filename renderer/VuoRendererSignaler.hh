/**
 * @file
 * VuoRendererSignaler interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERSIGNALER_HH
#define VUORENDERERSIGNALER_HH

class VuoRendererPort;
class VuoRendererPublishedPort;
class VuoRendererNode;
class VuoRendererCable;


/**
 * Sends signals on behalf of VuoRenderer objects that don't inherit from QObject.
 */
class VuoRendererSignaler : public QObject
{
	Q_OBJECT

public:
	explicit VuoRendererSignaler(void);
	void signalNodesMoved(set<VuoRendererNode *> nodes, qreal dx, qreal dy, bool movedByDragging=false);
	void signalInputEditorRequested(VuoRendererPort *port);
	void signalNodePopoverRequested(VuoRendererNode *node);
	void signalNodeTitleEditorRequested(VuoRendererNode *node);
	void signalSubcompositionEditRequested(VuoRendererNode *node);
	void signalInputPortCountAdjustmentRequested(VuoRendererNode *node, int inputPortCountDelta, bool requestedByDragging);
	void signalDisableDragStickiness(bool disable);

signals:
	void nodesMoved(set<VuoRendererNode *> nodes, qreal dx, qreal dy, bool movedByDragging); ///< Emitted when nodes have been moved.
	void inputEditorRequested(VuoRendererPort *port); ///< Emitted when the user has requested an input editor be presented.
	void nodePopoverRequested(VuoRendererNode *node); ///< Emitted when the user has requested a node popover be presented.
	void nodeTitleEditorRequested(VuoRendererNode *node); ///< Emitted when the user has requested a node title editor be presented.
	void subcompositionEditRequested(VuoRendererNode *node); ///< Emitted when the user has initiated an editing session for an installed subcomposition.
	void inputPortCountAdjustmentRequested(VuoRendererNode *node, int inputPortCountDelta, bool requestedByDragging); ///< Emitted when a node is to have its port count adjusted.
	void dragStickinessDisableRequested(bool disable); ///< Emitted when drag stickiness for the canvas is to be disabled or re-enabled.
};

#endif // VUORENDERERSIGNALER_HH
