/**
 * @file
 * VuoRendererNode interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERNODE_HH
#define VUORENDERERNODE_HH

#include "VuoRendererItem.hh"
#include "VuoRendererPortList.hh"
#include "VuoRendererSignaler.hh"

#include "VuoBaseDetail.hh"
#include "VuoNode.hh"

class VuoCompilerNode;
class VuoCompilerNodeClass;
class VuoRendererColors;
class VuoRendererMakeListNode;
class VuoRendererPort;
class VuoRendererTypecastPort;
class VuoNodeClass;
class VuoCable;

/**
 * Renders a node in a @c VuoRendererComposition.
 */
class VuoRendererNode : public VuoRendererItem, public VuoBaseDetail<VuoNode>
{
public:
	/**
	 * Specifies how this node is graphically represented.
	 */
	enum Type
	{
		node,
		detachedConstant,
		detachedDrawer,
		detachedTypecast
	};

	VuoRendererNode(VuoNode * baseNode, VuoRendererSignaler *signaler);

	virtual QRectF boundingRect(void) const;
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	virtual QRectF getNodeTitleBoundingRect(void) const;

	vector<VuoRendererMakeListNode *> getAttachedInputDrawers(void) const;
	qreal getInputDrawerOffset(unsigned int portIndex) const;
	void updateNodeFrameRect(void);

	void setMissingImplementation(bool missingImplementation);

	void setProxyNode(VuoRendererNode * proxyNode);
	VuoRendererNode * getProxyNode(void) const;
	VuoRendererTypecastPort * getProxyCollapsedTypecast(void) const;

	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
	void updateGeometry(void);
	void updateConnectedCableGeometry(void);

	set<VuoCable *> getConnectedCables(bool includePublishedCables);
	set<VuoCable *> getConnectedInputCables(bool includePublishedCables);
	set<VuoCable *> getConnectedOutputCables(bool includePublishedCables);
	VuoRendererPortList * getInputPorts(void);
	VuoRendererPortList * getOutputPorts(void);
	void replaceInputPort(VuoRendererPort * oldPort, VuoRendererPort * newPort);
	void addInputPort(VuoRendererPort * newPort);
	bool hasGenericPort(void);
	void setTitle(string title);

	static QString generateNodeClassToolTipTitle(VuoNodeClass *nodeClass);
	static QString generateNodeClassToolTipTextBody(VuoNodeClass *nodeClass);

	void resetTimeLastExecuted();
	void setExecutionEnded();
	void setExecutionBegun();
	qint64 getTimeLastExecutionEnded();
	void layoutConnectedInputDrawers(void);
	void layoutConnectedInputDrawersAtAndAbovePort(VuoRendererPort *port);

	// Drawing configuration
	static const qreal nodeTitleHeight;

protected:
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
	virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
	void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event);
	void keyPressEvent(QKeyEvent *event);

	virtual void layoutPorts(void);

	VuoRendererSignaler *signaler; ///< The object that sends signals on behalf of this renderer node.
	VuoRendererPortList *inputPorts; ///<  The list of input renderer ports belonging to this renderer node.
	VuoRendererPortList *outputPorts; ///< The list of output renderer ports belonging to this renderer node.
	qint64 timeLastExecutionEnded; ///< The time at which this node's running counterpart last completed an execution.

private:
	VuoRendererNode *proxyNode; ///< The optional node that handles the rendering for this node.

	// Drawing configuration
	static const qreal subcompositionBulge;
	static const qreal cornerRadius;
	static const qreal nodeClassHeight;

	QRectF frameRect;
	QRectF nodeTitleBoundingRect;

	// Node attributes affecting drawing
	enum Type nodeType;
	QString nodeTitle;
	QString nodeClass;
	bool nodeIsStateful;
	bool nodeIsSubcomposition;
	bool nodeIsMissing;
	VuoRendererPort *refreshPort;
	VuoRendererPort *functionPort;

	// Internal methods
	void initWithNodeClass(VuoNodeClass *nodeClass);
	void setInputPorts(vector<VuoRendererPort *> inputPorts = vector<VuoRendererPort *>());
	void setOutputPorts(vector<VuoRendererPort *> outputPorts = vector<VuoRendererPort *>());

	void drawNodeFrame(QPainter *painter, QRectF nodeInnerFrameRect, VuoRendererColors *colors) const;
	static void addRoundedCorner(QPainterPath &path, bool drawLine, QPointF sharpCornerPoint, qreal radius, bool isTop, bool isLeft);

	void layoutConnectedInputDrawer(unsigned int i);

	QPointF getPortPoint(VuoRendererPort *port, unsigned int portIndex) const;
};

#endif // VUORENDERERNODE_HH
