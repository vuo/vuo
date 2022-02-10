/**
 * @file
 * VuoRendererTypecastPort implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRendererTypecastPort.hh"

#include "VuoCompilerPortClass.hh"
#include "VuoNodeClass.hh"
#include "VuoPort.hh"
#include "VuoRendererFonts.hh"

const qreal VuoRendererTypecastPort::textPadding = 7 /*VuoRendererPort::portRadius - 1*/;  ///< Space left and right of the text, inside the rounded rect.

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
	updateCachedBoundingRect();
}

/**
 * Returns a closed path representing the collapsed typecast port's frame.
 */
QPainterPath VuoRendererTypecastPort::getPortPath(bool includeNormalPort, bool includeFlag) const
{
	QPainterPath p;

	// Draw a normal port...
	if (includeNormalPort)
		p += VuoRendererPort::getPortPath();

	// ...and add a rectangle connecting the host port to the child port.
	if (includeFlag)
	{
		qreal r = VuoRendererPort::portRadius - 2;
		p.addRoundedRect(getPortConstantTextRect().adjusted(-textPadding/2 - 1, .15, 0, -.85), r, r);
	}

	return p;
}

/**
 * Returns a rect enclosing the string representation of the port's typecast title.
 */
QRectF VuoRendererTypecastPort::getPortConstantTextRect(void) const
{
	return getPortConstantTextRectForText(getCanvasTypecastTitle())
			.adjusted(-VuoRendererPort::portRadius - textPadding/2 + 1, 0, -VuoRendererPort::portRadius, 0);
}

/**
 * Returns the number of pixels between the center of @c basePort and @c childPort.
 */
qreal VuoRendererTypecastPort::getChildPortXOffset(void) const
{
	return getPortConstantTextRect().width() + (VuoRendererPort::portRadius - 1)*2 - 1 + textPadding/2 + 1;
}

/**
 * Returns the title string for the typecast node attached to this port,
 * as it should be rendered on the canvas.
 */
QString VuoRendererTypecastPort::getCanvasTypecastTitle(void) const
{
	return getTypecastTitleForNodeClass(uncollapsedTypecastNode->getBase()->getNodeClass(), false);
}

/**
 * Returns the title string for a typecast node having the provided @c typecastSourceType and
 * @c typecastDestinationType port class names.  If @c inMenu is true, returns
 * the title as it should be rendered in a menu of typecast options.  Otherwise, returns
 * the title as it should be rendered on canvas.
 */
QString VuoRendererTypecastPort::getTypecastTitleForNodeClass(VuoNodeClass *typecastClass, bool inMenu)
{
	string typecastClassName = typecastClass->getClassName();

	// Check whether this typecast class has a custom title.
	map<string, string> customTypecastTitles;
	customTypecastTitles["vuo.type.real.boolean"] = "Real ≠ 0";
	customTypecastTitles["vuo.type.real.enum.VuoBoolean"] = "Real ≥ 0.5";

	map<string, string>::iterator typecastTitle = customTypecastTitles.find(typecastClassName);
	if (typecastTitle != customTypecastTitles.end())
		return typecastTitle->second.c_str();

	// If not, construct the typecast title from its input and output port names.
	VuoPortClass *typecastInPortClass = typecastClass->getInputPortClasses()[VuoNodeClass::unreservedInputPortStartIndex];
	VuoPortClass *typecastOutPortClass = typecastClass->getOutputPortClasses()[VuoNodeClass::unreservedOutputPortStartIndex];

	QString typecastInPortName = QString::fromStdString(static_cast<VuoCompilerPortClass *>(typecastInPortClass->getCompiler())->getDisplayName());
	QString typecastOutPortName = QString::fromStdString(static_cast<VuoCompilerPortClass *>(typecastOutPortClass->getCompiler())->getDisplayName());

	QString separator = (inMenu? QString::fromUtf8(" → ") : "     ");
	return typecastInPortName + separator + typecastOutPortName;
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
	return cachedBoundingRect;
}

void VuoRendererTypecastPort::updateCachedBoundingRect()
{
	QPainterPath path = getPortPath(true, true);
	cachedBoundingRect = path.controlPointRect();

	if (!isRefreshPort)
		cachedBoundingRect = cachedBoundingRect.united(getNameRect());

	if (hasPortAction())
		cachedBoundingRect = cachedBoundingRect.united(getActionIndicatorRect());

	// Antialiasing bleed
	cachedBoundingRect.adjust(-1,-1,1,1);

	cachedBoundingRect = cachedBoundingRect.toAlignedRect();
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

	VuoRendererColors::SelectionType selectionType = (getRenderedParentNode()->isEffectivelySelected()? VuoRendererColors::directSelection : VuoRendererColors::noSelection);
	qint64 timeOfLastActivity =	((! getRenderActivity())?	VuoRendererItem::notTrackingActivity :
															getUncollapsedTypecastNode()->getTimeLastExecutionEnded());

	VuoRendererColors *colors = new VuoRendererColors(getRenderedParentNode()->getBase()->getTintColor(),
													  selectionType,
													  false,
													  eligibilityHighlight(),
													  timeOfLastActivity);

	// Fill
	QPainterPath innerTypecastPath = getPortPath(false, true);
	painter->fillPath(innerTypecastPath, colors->nodeFill());

	// Typecast description
	QRectF textRect = getPortConstantTextRect();
	painter->setPen(colors->portTitle());
	painter->setFont(VuoRendererFonts::getSharedFonts()->nodePortConstantFont());
	painter->drawText(textRect, Qt::AlignLeft, getCanvasTypecastTitle());

	// Draw the normal port.
	VuoRendererPort::paint(painter,option,widget);

	delete colors;
}
