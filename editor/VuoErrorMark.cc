/**
 * @file
 * VuoErrorMark implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoErrorMark.hh"
#include "VuoRendererCable.hh"
#include "VuoRendererFonts.hh"
#include "VuoRendererTypecastPort.hh"

/**
 * Creates (but does not yet show) the error mark.
 */
VuoErrorMark::VuoErrorMark()
{
	setZValue(errorMarkZValue);
	this->errorMarkPath = QPainterPath();
}

/**
 * Adds the provided nodes and cables to the set of components to be encompassed in the error mark.
 */
void VuoErrorMark::addMarkedComponents(set<VuoRendererNode *> markedNodes, set<VuoRendererCable *> markedCables)
{
	foreach (VuoRendererNode *node, markedNodes)
		this->nodes.insert(node);

	foreach (VuoRendererCable *cable, markedCables)
		this->cables.insert(cable);

	updateErrorMarkPath();
}

/**
 * Returns a bounding rect that encompasses all nodes and cables marked.
 */
QRectF VuoErrorMark::boundingRect(void) const
{
	QPainterPath errorMarkPath = getErrorMarkPath();
	QRectF r = errorMarkPath.boundingRect();

	// Antialiasing bleed
	r.adjust(-1,-1,1,1);

	return r.toAlignedRect();
}

/**
 * Paints a red outline around the nodes and cables.
 *
 * Assumes this VuoErrorMark will be added to the QGraphicsScene at a z level behind the nodes and cables.
 */
void VuoErrorMark::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	// Workaround to prevent items that have been removed from the scene from being painted on the scene anyway.
	// https://b33p.net/kosada/node/8267
	if (!scene())
		return;

	drawBoundingRect(painter);

	QPainterPath path = getErrorMarkPath();

	VuoRendererColors colors;
	painter->fillPath(path, colors.errorMark());
}

/**
 * Removes the error mark from the canvas and performs other necessary cleanup.
 */
void VuoErrorMark::removeFromScene()
{
	QGraphicsScene *itemScene = scene();
	if (itemScene)
	{
		this->prepareGeometryChange();
		itemScene->removeItem(this);
	}
}

/**
 * Calculates and updates the cached painter path of the error mark
 * based on its current set of included nodes and cables.
 */
void VuoErrorMark::updateErrorMarkPath(void)
{
	this->prepareGeometryChange();

	QPainterPath path;
	const qreal markWidth = VuoRendererFonts::thickPenWidth / 2.;
	const qreal cornerRadius = VuoRendererNode::cornerRadius + markWidth;

	foreach (VuoRendererNode *node, nodes)
	{
		QPainterPath nodeSimplifiedOutline;

		VuoRendererTypecastPort *typecastPort = node->getProxyCollapsedTypecast();
		if (typecastPort)
		{
			QRectF typecastPortRect = typecastPort->boundingRect();
			if (!typecastPortRect.isNull())
			{
				QPainterPath typecastPath;
				typecastPath.addRoundedRect( typecastPortRect.adjusted(-0.5*markWidth, -0.5*markWidth, 0.5*markWidth, 0.5*markWidth),
													  cornerRadius, cornerRadius );
				typecastPath.translate( typecastPort->scenePos() );
				path = path.united( typecastPath );
			}
		}

		else
		{
			QRectF nodeOuterFrameRect = node->boundingRect();
			if (!nodeOuterFrameRect.isNull())
			{
				nodeSimplifiedOutline.addRoundedRect( nodeOuterFrameRect.adjusted(-markWidth, -markWidth, markWidth, markWidth),
													  cornerRadius, cornerRadius );

				nodeSimplifiedOutline.translate( node->getBase()->getX(), node->getBase()->getY() );
				path = path.united( nodeSimplifiedOutline );
			}
		}
	}

	foreach (VuoRendererCable *cable, cables)
	{
		QPainterPathStroker stroker;
		stroker.setWidth(markWidth);
		QPainterPath cableCurve = stroker.createStroke( cable->getCablePath() );
		path = path.united( cableCurve );
	}

	this->errorMarkPath = path;
}

/**
 * Returns a closed path representing the error mark.
 */
QPainterPath VuoErrorMark::getErrorMarkPath() const
{
	return this->errorMarkPath;
}
