/**
 * @file
 * VuoRendererNode interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoRendererItem.hh"
#include "VuoRendererColors.hh"

#include "VuoBaseDetail.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
	#include <json-c/json.h>
#pragma clang diagnostic pop

class VuoRendererInputDrawer;
class VuoRendererPort;
class VuoRendererSignaler;
class VuoRendererTypecastPort;
class VuoNode;
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
	virtual QRectF getOuterNodeFrameBoundingRect(void) const;
	virtual bool paintingDisabled(void) const;
	virtual bool isEffectivelySelected(void);

	vector<VuoRendererInputDrawer *> getAttachedInputDrawers(void) const;
	qreal getInputDrawerOffset(unsigned int portIndex) const;
	void updateNodeFrameRect(void);

	static QPair<QPainterPath, QPainterPath> getNodeFrames(QRectF nodeInnerFrameRect);
	static QPainterPath getSubcompositionIndicatorPath(QRectF nodeInnerFrameRect, bool isSubcomposition);

	void setMissingImplementation(bool missingImplementation);
	bool isMissingImplementation(void);

	void setProxyNode(VuoRendererNode * proxyNode);
	VuoRendererNode * getProxyNode(void) const;
	VuoRendererTypecastPort * getProxyCollapsedTypecast(void) const;
	void setAlwaysDisplayPortNames(bool displayPortNames);
	bool nameDisplayEnabledForPort(const VuoRendererPort *port);
	bool nameDisplayEnabledForInputPorts();
	bool nameDisplayEnabledForOutputPorts();
	VuoRendererColors::HighlightType getEligibilityHighlight(void);
	void setEligibilityHighlight(VuoRendererColors::HighlightType eligibility);

	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
	void updateGeometry(void);
	void updateConnectedCableGeometry(void);

	set<VuoCable *> getConnectedCables(bool includePublishedCables);
	set<VuoCable *> getConnectedInputCables(bool includePublishedCables);
	set<VuoCable *> getConnectedOutputCables(bool includePublishedCables);
	vector<VuoRendererPort *> &getInputPorts(void);
	vector<VuoRendererPort *> &getOutputPorts(void);
	vector<pair<QString, json_object *> > getConstantPortValues();
	void replaceInputPort(VuoRendererPort * oldPort, VuoRendererPort * newPort);
	void addInputPort(VuoRendererPort * newPort);
	bool hasGenericPort(void);
	void setTitle(string title);

	void resetTimeLastExecuted();
	void setExecutionEnded();
	void setExecutionBegun();
	qint64 getTimeLastExecutionEnded();
	void layoutConnectedInputDrawers(void);
	void layoutConnectedInputDrawersAtAndAbovePort(VuoRendererPort *port);
	void setCacheModeForNodeAndPorts(QGraphicsItem::CacheMode mode);
	void setCacheModeForConnectedCables(QGraphicsItem::CacheMode mode);

	// Drawing configuration
	static const qreal nodeTitleHeight;
	static const qreal nodeHeaderYOffset;
	static const qreal cornerRadius;

protected:
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
	virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event);
	void keyPressEvent(QKeyEvent *event);

	// Drawing configuration
	virtual void layoutPorts(void);

	VuoRendererSignaler *signaler; ///< The object that sends signals on behalf of this renderer node.
	vector<VuoRendererPort *> inputPorts; ///<  The list of input renderer ports belonging to this renderer node.
	vector<VuoRendererPort *> outputPorts; ///< The list of output renderer ports belonging to this renderer node.
	qint64 timeLastExecutionEnded; ///< The time at which this node's running counterpart last completed an execution.
	bool alwaysDisplayPortNames; ///< Indicates whether this node's ports, even unambiguous ones, should always have their names displayed.
	VuoRendererColors::HighlightType _eligibilityHighlight; ///< Indicates whether this node contains ports eligible for the current cable drag.

private:
	VuoRendererNode *proxyNode; ///< The optional node that handles the rendering for this node.

	// Drawing configuration
	static const qreal outerBorderWidth;
	static const qreal nodeTitleHorizontalMargin;
	static const qreal nodeClassHeight;
	static const qreal iconRightOffset;

	QRectF frameRect;
	QPair<QPainterPath, QPainterPath> nodeFrames; // (nodeOuterFrame, nodeInnerFrame)
	QPainterPath subcompositionIndicatorPath;

	QRectF nodeTitleBoundingRect;

	// Node attributes affecting drawing
	enum Type nodeType;
	QString nodeClass;
	bool nodeIsSubcomposition;
	bool nodeIsMissing;
	VuoRendererPort *refreshPort;
	VuoRendererPort *functionPort;

	// Internal methods
	void initWithNodeClass(VuoNodeClass *nodeClass);
	void setInputPorts(vector<VuoRendererPort *> inputPorts = vector<VuoRendererPort *>());
	void setOutputPorts(vector<VuoRendererPort *> outputPorts = vector<VuoRendererPort *>());

	void drawNodeFrame(QPainter *painter, QRectF nodeInnerFrameRect, VuoRendererColors *colors) const;
	void layoutConnectedInputDrawer(unsigned int i);

	QPointF getPortPoint(VuoRendererPort *port, unsigned int portIndex) const;
};
Q_DECLARE_METATYPE(VuoRendererNode *)
