/**
 * @file
 * VuoRendererMakeListNode interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERMAKELISTNODE_HH
#define VUORENDERERMAKELISTNODE_HH

#include "VuoRendererNode.hh"

#include <vector>
using namespace std;

class VuoRendererPort;

/**
 * Represents the compact form of a "Make List" node.
 */
class VuoRendererMakeListNode : public VuoRendererNode
{
public:
	VuoRendererMakeListNode(VuoNode *baseNode, VuoRendererSignaler *signaler);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	QRectF boundingRect(void) const;
	QPainterPath shape() const;

	vector<VuoRendererPort *> getDrawerPorts(void) const;
	VuoRendererPort * getHostInputPort(void) const;
	qreal getMaxDrawerLabelWidth(void) const;
	qreal getMaxDrawerChainedLabelWidth(void) const;
	void setHorizontalDrawerOffset(qreal offset);
	void setDragInProgress(bool inProgress);

	static const qreal drawerHorizontalSpacing; ///< The amount of space, in pixels, left as horizontal padding between underhangs of drawers attached to ports of the same node.

private:
	QPainterPath getMakeListNodePath() const;
	QPainterPath getMakeListDrawerPath() const;
	QPainterPath getMakeListDragHandlePath() const;
	QRectF getDragHandleRect() const;
	QRectF getExtendedDragHandleRect() const;
	void triggerPortCountAdjustment(int portCountDelta, QGraphicsSceneMouseEvent *event);
	void layoutPorts(void);

	void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
	void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

	vector<VuoRendererPort *> drawerPorts; ///< The vector of input ports whose values will be incorporated into the output list.
	qreal horizontalDrawerOffset; ///< The distance, in pixels, left of its attached port that the rightmost point of this drawer should be rendered.
	qreal drawerBottomExtensionHeight; ///< The height, in pixels, of the input drawer (excluding the arm and drag handle).
	static const qreal drawerInteriorHorizontalPadding; ///< The amount of horizontal padding, in pixels, added to each drawer beyond what its text strictly requires.

	// User interaction with drag handle
	bool dragInProgress;
	QPointF mousePositionAtLastPortCountAdjustment;
	bool dragHandleIsHovered;
};

#endif // VUORENDERERMAKELISTNODE_HH
