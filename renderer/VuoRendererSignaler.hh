/**
 * @file
 * VuoRendererSignaler interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoRendererPort;
class VuoRendererNode;
class VuoRendererComment;

/**
 * Sends signals on behalf of VuoRenderer objects that don't inherit from QObject.
 */
class VuoRendererSignaler : public QObject
{
	Q_OBJECT

public:
	explicit VuoRendererSignaler(void);
	void signalNodesMoved(set<VuoRendererNode *> nodes, qreal dx, qreal dy, bool movedByDragging=false);
	void signalCommentsMoved(set<VuoRendererComment *> comments, qreal dx, qreal dy, bool movedByDragging=false); // @todo https://b33p.net/kosada/node/9986
	void signalInputEditorRequested(VuoRendererPort *port);
	void signalNodePopoverRequested(VuoRendererNode *node);
	void signalNodeTitleEditorRequested(VuoRendererNode *node);
	void signalCommentEditorRequested(VuoRendererComment *comment);
	void signalCommentZoomRequested(VuoRendererComment *comment);
	void signalCommentResized(VuoRendererComment *comment, qreal dx, qreal dy);
	void signalNodeSourceEditorRequested(VuoRendererNode *node);
	void signalInputPortCountAdjustmentRequested(VuoRendererNode *node, int inputPortCountDelta, bool requestedByDragging);
	void signalDisableDragStickiness(bool disable);
	void signalOpenUrl(QString url);

signals:
	void nodesMoved(set<VuoRendererNode *> nodes, qreal dx, qreal dy, bool movedByDragging); ///< Emitted when nodes have been moved.
	void commentsMoved(set<VuoRendererComment *> comments, qreal dx, qreal dy, bool movedByDragging); ///< Emitted when comments have been moved. // @todo https://b33p.net/kosada/node/9986 merge with above
	void inputEditorRequested(VuoRendererPort *port); ///< Emitted when the user has requested an input editor be presented.
	void nodePopoverRequested(VuoRendererNode *node); ///< Emitted when the user has requested a node popover be presented.
	void nodeTitleEditorRequested(VuoRendererNode *node); ///< Emitted when the user has requested a node title editor be presented.
	void commentEditorRequested(VuoRendererComment *comment); ///< Emitted when the user has requested a comment editor be presented.
	void commentZoomRequested(VuoRendererComment *comment); ///< Emitted when the user has requested a comment to be zoomed-to-fit.
	void commentResized(VuoRendererComment *comment, qreal dx, qreal dy); ///< Emitted when the user has manually resized a comment.
	void nodeSourceEditorRequested(VuoRendererNode *node); ///< Emitted when the user has initiated an editing session for a node with editable source.
	void inputPortCountAdjustmentRequested(VuoRendererNode *node, int inputPortCountDelta, bool requestedByDragging); ///< Emitted when a node is to have its port count adjusted.
	void dragStickinessDisableRequested(bool disable); ///< Emitted when drag stickiness for the canvas is to be disabled or re-enabled.
	void openUrl(QString url); ///< Emitted when a URL should be opened.
};

