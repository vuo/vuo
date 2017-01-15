/**
 * @file
 * VuoRendererInputDrawer implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererInputDrawer.hh"
#include "VuoRendererPort.hh"
#include "VuoRendererFonts.hh"
#include "VuoRendererTypecastPort.hh"
#include "VuoNodeClass.hh"

const qreal VuoRendererInputDrawer::drawerInteriorHorizontalPadding = VuoRendererFonts::thickPenWidth*3./4.+5;
const qreal VuoRendererInputDrawer::drawerHorizontalSpacing = VuoRendererFonts::thickPenWidth*1./4.;

/**
 * Creates a collapsed "Make List" node that takes the form of an input drawer.
 */
VuoRendererInputDrawer::VuoRendererInputDrawer(VuoNode *baseNode, VuoRendererSignaler *signaler)
	: VuoRendererInputAttachment(baseNode, signaler)
{
	vector<VuoPort *> inputPorts = this->getBase()->getInputPorts();
	for (int i = VuoNodeClass::unreservedInputPortStartIndex; i < inputPorts.size(); ++i)
		drawerPorts.push_back(inputPorts[i]->getRenderer());

	QPainterPath outsetPath;
	VuoRendererPort::getPortConstantPath(VuoRendererPort::getPortRect(), QString::fromUtf8(""),&outsetPath);
	qreal minimumDrawerArmLength = outsetPath.boundingRect().width();
	this->horizontalDrawerOffset = 0.5*VuoRendererPort::getPortRect().width() + minimumDrawerArmLength + getMaxDrawerLabelWidth();
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
	QFont labelFont = VuoRendererFonts::getSharedFonts()->nodePortConstantFont();
	foreach (VuoRendererPort *port, drawerPorts)
	{
		string portTitle = port->getPortNameToRender();
		qreal labelWidth = QFontMetricsF(labelFont).boundingRect(QString::fromUtf8(portTitle.c_str())).width();

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
	qreal maxLabelWidth = 0;
	QFont labelFont = VuoRendererFonts::getSharedFonts()->nodePortConstantFont();
	foreach (VuoRendererPort *port, drawerPorts)
	{
		string portTitle = port->getPortNameToRender();
		qreal labelWidth = QFontMetricsF(labelFont).boundingRect(QString::fromUtf8(portTitle.c_str())).width();

		// Accommodate the width of the child port.
		labelWidth += 0.5*VuoRendererPort::getPortRect().width();

		// Accommodate the width of any collapsed typecast attached to the child port.
		if (dynamic_cast<VuoRendererTypecastPort *>(port->getBase()->getRenderer()))
		{
			VuoRendererTypecastPort *tp = (VuoRendererTypecastPort *)(port->getBase()->getRenderer());
			QPainterPath outsetPath;
			tp->getPortPath(false, true, &outsetPath);
			labelWidth += outsetPath.boundingRect().width();

			// Accommodate the width of the attached typecast's child port.
			labelWidth += 0.5*VuoRendererPort::getPortRect().width();
		}
		else

		// Accommodate the width of any constant flag attached to the child port.
		if (port->isConstant())
		{
			QPainterPath outsetPath;
			port->getPortConstantPath(VuoRendererPort::getPortRect(), QString::fromUtf8(port->getConstantAsStringToRender().c_str()),&outsetPath);
			labelWidth += outsetPath.boundingRect().width();
		}

		if (labelWidth > maxLabelWidth)
			maxLabelWidth = labelWidth;
	}

	return maxLabelWidth + drawerInteriorHorizontalPadding;
}

/**
  * Calculates and sets the positions of the node's child ports relative to the node.
  */
void VuoRendererInputDrawer::layoutPorts(void)
{
	QList<VuoRendererPort *> inputPorts = this->inputPorts->childItems();
	for (unsigned int i = 0; i < inputPorts.size(); ++i)
	{
		VuoRendererPort *p = inputPorts[i];

		if (i < VuoNodeClass::unreservedInputPortStartIndex)
			p->setVisible(false);

		else
		{
			p->setVisible(true);
			int adjustedPortIndex = i-VuoNodeClass::unreservedInputPortStartIndex;
			qreal portPointY = adjustedPortIndex*VuoRendererPort::portSpacing;
			p->setPos(QPointF(0,portPointY));

			VuoRendererTypecastPort *tp = dynamic_cast<VuoRendererTypecastPort *>(p);
			if (tp)
				tp->getChildPort()->setPos(QPointF(-tp->getChildPortXOffset(),portPointY));
		}
	}

	// Do not display output ports.
	QList<VuoRendererPort *> outputPorts = this->outputPorts->childItems();
	for (unsigned int i = 0; i < outputPorts.size(); ++i)
		outputPorts[i]->setVisible(false);
}

/**
 * Returns the vector of the input ports whose values will be incorporated into the output list.
 */
vector<VuoRendererPort *> VuoRendererInputDrawer::getDrawerPorts(void) const
{
	return drawerPorts;
}
