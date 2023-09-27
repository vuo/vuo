/**
 * @file
 * VuoRendererInputDrawer implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRendererInputDrawer.hh"
#include "VuoRendererFonts.hh"
#include "VuoRendererTypecastPort.hh"
#include "VuoNodeClass.hh"
#include "VuoPort.hh"

const qreal VuoRendererInputDrawer::drawerInteriorHorizontalPadding = 20;  // VuoRendererFonts::thickPenWidth*3./4.+5
const qreal VuoRendererInputDrawer::drawerHorizontalSpacing         = 2;   // VuoRendererFonts::thickPenWidth*2./20.

/**
 * Creates a collapsed "Make List" node that takes the form of an input drawer.
 */
VuoRendererInputDrawer::VuoRendererInputDrawer(VuoNode *baseNode, VuoRendererSignaler *signaler)
	: VuoRendererInputAttachment(baseNode, signaler)
{
	vector<VuoPort *> inputPorts = this->getBase()->getInputPorts();
	for (int i = VuoNodeClass::unreservedInputPortStartIndex; i < inputPorts.size(); ++i)
		drawerPorts.push_back(inputPorts[i]->getRenderer());

	this->horizontalDrawerOffset = 0.5*VuoRendererPort::getPortRect().width() + getMaxDrawerLabelWidth();
	this->drawerBottomExtensionHeight = VuoRendererPort::portSpacing*(fmax(1,drawerPorts.size())-1);

	layoutPorts();
}

/**
  * Sets the distance, in pixels, left of its attached port that the rightmost point of this drawer should be rendered.
  */
void VuoRendererInputDrawer::setHorizontalDrawerOffset(qreal offset)
{
	this->horizontalDrawerOffset = offset;
}

/**
 * Returns the maximum width in pixels of the port labels within this drawer.
 */
qreal VuoRendererInputDrawer::getMaxDrawerLabelWidth(void) const
{
	qreal maxLabelWidth = 0;
	foreach (VuoRendererPort *port, drawerPorts)
	{
		string portTitle = port->getPortNameToRender();
		qreal labelWidth = VuoRendererPort::getTextWidth(QString::fromUtf8(portTitle.c_str()));

		if (labelWidth > maxLabelWidth)
			maxLabelWidth = labelWidth;
	}

	return maxLabelWidth + drawerInteriorHorizontalPadding;
}

/**
 * Returns the maximum width in pixels of the port labels within this drawer,
 * also accounting for the width of the child port as well as any constant flags
 * or collapsed typecasts attached to the child port.
 */
qreal VuoRendererInputDrawer::getMaxDrawerChainedLabelWidth(void) const
{
	qreal maxPortWidth = -10;
	foreach (VuoRendererPort *port, drawerPorts)
	{
		qreal portWidth = port->boundingRect().width() - VuoRendererPort::portRadius*2;

		// Accommodate the width of any collapsed typecast attached to the child port.
		if (dynamic_cast<VuoRendererTypecastPort *>(port->getBase()->getRenderer()))
		{
			VuoRendererTypecastPort *tp = (VuoRendererTypecastPort *)(port->getBase()->getRenderer());
			portWidth = tp->getPortPath(true, true).boundingRect().width() + VuoRendererPort::portRadius*3 + 5;
		}
		else if (!port->getBase()->getConnectedCables(true).empty()) // Leave room for the cable's rounded corner.
			portWidth += VuoRendererPort::portRadius + 3;

		if (portWidth > maxPortWidth)
			maxPortWidth = portWidth;
	}

	return maxPortWidth + drawerInteriorHorizontalPadding + VuoRendererPort::portRadius;
}

/**
  * Calculates and sets the positions of the node's child ports relative to the node.
  */
void VuoRendererInputDrawer::layoutPorts(void)
{
	unsigned int i = 0;
	for (vector<VuoRendererPort *>::iterator it = inputPorts.begin(); it != inputPorts.end(); ++it, ++i)
	{
		VuoRendererPort *p = *it;

		if (i < VuoNodeClass::unreservedInputPortStartIndex)
			p->setVisible(false);

		else
		{
			p->setVisible(true);
			int adjustedPortIndex = i-VuoNodeClass::unreservedInputPortStartIndex;
			qreal portPointY = adjustedPortIndex*VuoRendererPort::portSpacing - .15;
			p->setPos(QPointF(0,portPointY));

			VuoRendererTypecastPort *tp = dynamic_cast<VuoRendererTypecastPort *>(p);
			if (tp)
				tp->getChildPort()->setPos(QPointF(-tp->getChildPortXOffset(),portPointY));
		}
	}

	// Do not display output ports.
	for (vector<VuoRendererPort *>::iterator it = outputPorts.begin(); it != outputPorts.end(); ++it)
		(*it)->setVisible(false);
}

/**
 * Returns the vector of the input ports whose values will be incorporated into the output list.
 */
vector<VuoRendererPort *> VuoRendererInputDrawer::getDrawerPorts(void) const
{
	return drawerPorts;
}

/**
 * Returns a path representing the drawer.
 */
QPainterPath VuoRendererInputDrawer::getDrawerPath(bool includeDragHandle) const
{
	QRectF hostPortRect = VuoRendererPort::getPortRect();
	qreal drawerBottomExtensionWidth = qRound(getMaxDrawerLabelWidth());

	QPainterPath p;

	// Arm right edge (hide behind the parent port)
	p.moveTo(horizontalDrawerOffset, -0.5*hostPortRect.height() + 1);
	p.lineTo(horizontalDrawerOffset,  0.5*hostPortRect.height() - 2);

	// Arm bottom edge
	p.lineTo(drawerBottomExtensionWidth,  0.5*hostPortRect.height() - 2);

	// Right drawer wall
	qreal adjustedPortRadius = VuoRendererPort::portRadius - 2;
	qreal drawerBottomExtensionHeight = this->drawerBottomExtensionHeight + (includeDragHandle ? adjustedPortRadius : 0);
	if (drawerBottomExtensionHeight > 0)
		addRoundedCorner(p, true, QPointF(drawerBottomExtensionWidth, 0.5*hostPortRect.height() + 1 + drawerBottomExtensionHeight - 3), adjustedPortRadius, false, false);

	// Far drawer bottom
	addRoundedCorner(p, true, QPointF(hostPortRect.width()/2 - 1, 0.5*hostPortRect.height() + 1 + drawerBottomExtensionHeight - 3), adjustedPortRadius, false, true);

	// Drawer top left
	addRoundedCorner(p, true, QPointF(hostPortRect.width()/2 - 1, -0.5*hostPortRect.height() + 1), adjustedPortRadius, true, true);

	return p;
}
