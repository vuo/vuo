/**
 * @file
 * VuoRendererTypecastPort implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererTypecastPort.hh"

#include "VuoNodeClass.hh"
#include "VuoPort.hh"
#include "VuoPortClass.hh"
#include "VuoRendererNode.hh"
#include "VuoRendererInputDrawer.hh"
#include "VuoRendererColors.hh"
#include "VuoRendererFonts.hh"

/**
 * Creates a typecast port.
 *
 * @param uncollapsedTypecastNode The uncollapsed typecast node that this collapsed typecast port represents.
 * @param replacedPort The host input port replaced by this collapsed typecast.
 * @param signaler The signaler object, for sending Qt change notifications.
 */
VuoRendererTypecastPort::VuoRendererTypecastPort(VuoRendererNode *uncollapsedTypecastNode,
												 VuoRendererPort *replacedPort,
												 VuoRendererSignaler *signaler)
	: VuoRendererPort(replacedPort->getBase(),
					  signaler,
					  false,
					  replacedPort->getRefreshPort(),
					  false)
{
	this->replacedPort = replacedPort;
	this->setPortNameToRender(replacedPort->getPortNameToRender());
	setUncollapsedTypecastNode(uncollapsedTypecastNode);
}

/**
 * Sets the uncollapsed typecast node associated with this collapsed form, along with information
 * derived from the uncollapsed node that is necessary for the rendering of the collapsed form.
 */
void VuoRendererTypecastPort::setUncollapsedTypecastNode(VuoRendererNode *uncollapsedTypecastNode)
{
	// @todo: Handle this elsewhere.
	//setToolTip(uncollapsedTypecastNode->generateNodeClassToolTip(uncollapsedTypecastNode->getBase()->getNodeClass()));

	VuoPort *typecastInPort = uncollapsedTypecastNode->getBase()->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex];
	VuoPort *typecastOutPort = uncollapsedTypecastNode->getBase()->getOutputPorts()[VuoNodeClass::unreservedOutputPortStartIndex];

	this->uncollapsedTypecastNode = uncollapsedTypecastNode;
	this->sourceType = QString::fromStdString(typecastInPort->getRenderer()->getPortNameToRenderWhenDisplayed());
	this->destinationType = QString::fromStdString(typecastOutPort->getRenderer()->getPortNameToRenderWhenDisplayed());
	this->childPort = typecastInPort->getRenderer();
}

/**
 * Returns a closed path representing the collapsed typecast port's frame.
 */
QPainterPath VuoRendererTypecastPort::getPortPath(bool includeNormalPort, bool includeFlag, QPainterPath *outsetPath) const
{
	QPainterPath p;

	// Draw a normal port...
	if (includeNormalPort)
	{
		p += VuoRendererPort::getPortPath(VuoRendererPort::portInset);

		if (outsetPath)
			*outsetPath += VuoRendererPort::getPortPath(0);
	}

	// ...and add a constant flag to it (since typecast ports look basically like constant flags).
	if (includeFlag)
	{
		// Create a hybrid rect having the width of the port's inset rect and the customized
		// height of a constant flag, so that the collapsed typecast has the desired height but
		// directly adjoins the inset port shape.
		QRectF normalPortInnerRect = VuoRendererPort::getPortPath(VuoRendererPort::portInset).boundingRect();
		QRectF normalPortHybridRect = QRectF(normalPortInnerRect.x(), -0.5*VuoRendererPort::constantFlagHeight, normalPortInnerRect.width(), VuoRendererPort::constantFlagHeight);

		QPainterPath outsetPathTemp;
		p += getPortConstantPath(normalPortHybridRect, getCanvasTypecastTitle(), &outsetPathTemp, true);

		// @ todo: Replace magic 0/1 childPortXPadding value with something more transparent.
		VuoRendererNode *parentNode = getRenderedParentNode();
		const double childPortXPadding = (dynamic_cast<VuoRendererInputDrawer *>(parentNode)? 0 : 1);

		if(childPort)
			p -= childPort->getPortPath(0).translated(childPort->pos().x()+childPortXPadding, 0);

		if (outsetPath)
		{
			*outsetPath += outsetPathTemp;

			if(childPort)
				*outsetPath -= childPort->getPortPath(VuoRendererPort::portInset).translated(childPort->pos().x()+childPortXPadding, 0);
		}
	}

	return p;
}

/**
 * Returns a rect enclosing the string representation of the port's typecast title.
 */
QRectF VuoRendererTypecastPort::getPortConstantTextRect(void) const
{
	return getPortConstantTextRectForText(getCanvasTypecastTitle());
}

/**
 * Returns the number of pixels between the center of @c basePort and @c childPort.
 */
qreal VuoRendererTypecastPort::getChildPortXOffset(void) const
{
	return getPortConstantTextRectForText(getCanvasTypecastTitle()).width() + VuoRendererFonts::thickPenWidth + 2;
}

/**
 * Returns the title string for the typecast node attached to this port,
 * as it should be rendered on the canvas.
 */
QString VuoRendererTypecastPort::getCanvasTypecastTitle(void) const
{
	return getTypecastTitleForPorts(sourceType, destinationType, false);
}

/**
 * Returns the title string for a typecast node having the provided @c typecastSourceType and
 * @c typecastDestinationType port class names.  If @c inMenu is true, returns
 * the title as it should be rendered in a menu of typecast options.  Otherwise, returns
 * the title as it should be rendered on canvas.
 */
QString VuoRendererTypecastPort::getTypecastTitleForPorts(QString typecastSourcePort, QString typecastDestinationPort, bool inMenu)
{
	QString separator = (inMenu? QString::fromUtf8(" → ") : "     ");
	return typecastSourcePort + separator + typecastDestinationPort;
}

/**
 * Returns the uncollapsed typecast node from which this typecast port was derived.
 */
VuoRendererNode * VuoRendererTypecastPort::getUncollapsedTypecastNode(void) const
{
	return uncollapsedTypecastNode;
}

/**
 * Returns the port of the left side of the collapsed typecast port (the original typecast node's input port).
 */
VuoRendererPort * VuoRendererTypecastPort::getChildPort(void) const
{
	return childPort;
}

/**
 * Returns the target port that was replaced by this collapsed typecast.
 */
VuoRendererPort * VuoRendererTypecastPort::getReplacedPort(void) const
{
	return replacedPort;
}

/**
 * Returns a rectangle containing the rendered typecast port.
 */
QRectF VuoRendererTypecastPort::boundingRect(void) const
{
	QPainterPath outsetPath;
	getPortPath(true,true,&outsetPath);
	QRectF r = outsetPath.controlPointRect();

	if (!isRefreshPort)
		r = r.united(getNameRect());

	if (hasPortAction())
		r = r.united(getActionIndicatorRect());

	// Antialiasing bleed
	r.adjust(-1,-1,1,1);

	return r.toAlignedRect();
}

/**
 * Displays the typecast port.
 */
void VuoRendererTypecastPort::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (getRenderedParentNode()->getProxyNode())
		return;

	// Draw the collapsed typecast...

	drawBoundingRect(painter);

	VuoRendererColors::SelectionType selectionType = (getRenderedParentNode()->isSelected()? VuoRendererColors::directSelection : VuoRendererColors::noSelection);
	qint64 timeOfLastActivity =	((! getRenderActivity())?	VuoRendererItem::notTrackingActivity :
															getUncollapsedTypecastNode()->getTimeLastExecutionEnded());

	VuoRendererColors *colors = new VuoRendererColors(getRenderedParentNode()->getBase()->getTintColor(),
													  selectionType,
													  false,
													  VuoRendererColors::noHighlight,
													  timeOfLastActivity);

	// Fill
	QPainterPath outerTypecastPath;
	QPainterPath innerTypecastPath = getPortPath(false,true,&outerTypecastPath);
	painter->fillPath(innerTypecastPath, colors->nodeFill());
	outerTypecastPath -= innerTypecastPath;
	painter->fillPath(outerTypecastPath, colors->nodeFrame());

	// Typecast description
	QRectF textRect = getPortConstantTextRectForText(getCanvasTypecastTitle());
	painter->setPen(colors->portTitle());
	painter->setFont(VuoRendererFonts::getSharedFonts()->nodePortConstantFont());
	painter->drawText(textRect, Qt::AlignLeft, getCanvasTypecastTitle());

	// Draw the normal port.
	VuoRendererPort::paint(painter,option,widget);

	delete colors;
}
