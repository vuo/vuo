/**
 * @file
 * VuoRendererPublishedPort implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoPortClass.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoRendererPublishedPort.hh"
#include "VuoRendererPort.hh"

/**
 * Creates a published input or output port.
 *
 * @param basePublishedPort The base for which this renderer detail is to be created.
 */
VuoRendererPublishedPort::VuoRendererPublishedPort(VuoPublishedPort *basePublishedPort)
	: VuoBaseDetail<VuoPublishedPort>("VuoRendererPublishedPort", basePublishedPort)
{
	getBase()->setRenderer(this);
	this->globalPos = QPointF();
	setFlag(QGraphicsItem::ItemIsSelectable, true);
}

/**
 * Draws the published port on @c painter.
 */
void VuoRendererPublishedPort::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	VuoPort *firstConnectedPort = (*(getBase()->getConnectedPorts().begin()));
	if (! firstConnectedPort)
		return;

	firstConnectedPort->getRenderer()->paintWithOptions(painter, true);
}

/**
 * Returns a rectangle containing the rendered published port.
 */
QRectF VuoRendererPublishedPort::boundingRect(void) const
{
	VuoPort *firstConnectedPort = (*(getBase()->getConnectedPorts().begin()));
	if (! firstConnectedPort)
		return QRectF();

	return firstConnectedPort->getRenderer()->boundingRectWithOptions(true);
}

/**
 * Returns a boolean indicating whether a new @c internalPort may be
 * attached to/from this externally visible published port without
 * displacing any currently connected internal data ports.
 */
bool VuoRendererPublishedPort::canAccommodateInternalPort(VuoRendererPort *internalPort)
{
	if (! isCompatibleAliasForInternalPort(internalPort))
		return false;

	// If this is an output port already acting as an alias for an internal port
	// carrying data, it cannot accommodate another connected data port.
	if (getBase()->getOutput() && internalPort->getDataType())
	{
		foreach (VuoPort *connectedPort, getBase()->getConnectedPorts())
			if (connectedPort->getRenderer()->getDataType())
				return false;
	}

	return true;
}

/**
 * Returns a boolean indicating whether there may be a cable
 * attached directly between this externally visible published port
 * and the input @c internalPort, taking into account
 * the respective port types (input vs. output; event-only vs.
 * event+data; respective data types).
 */
bool VuoRendererPublishedPort::isCompatibleAliasForInternalPort(VuoRendererPort *internalPort)
{
	// For now, simply check whether the types of the internal and
	// external ports match exactly.
	// @todo: Implement more sophisticated type compatibility checks.
	// See https://b33p.net/kosada/node/4695

	// Case: Assigning a published input port alias to an internal input port
	if (getBase()->getInput() && internalPort->getInput())
		return (this->getBase()->getType() == internalPort->getDataType());

	// Case: Assigning a published output port alias to an internal output port
	else if (getBase()->getOutput() && internalPort->getOutput())
		return (this->getBase()->getType() == internalPort->getDataType());

	// Case: Published port alias and internal port have incompatible input/output types
	else
		return false;
}

/**
 * Returns a boolean indicating whether the @c otherExternalPort may be
 * merged with this one, taking into account
 * the respective port types (input vs. output; event-only vs.
 * event+data; respective data types), without
 * displacing any currently connected internal data ports.
 */
bool VuoRendererPublishedPort::canBeMergedWith(VuoPublishedPort *otherExternalPort)
{
	// For these two externally visible published ports to be eligible for merging, they must have the same type.
	if (! ((this->getBase()->getInput() == otherExternalPort->getInput()) &&
		   (this->getBase()->getType() == otherExternalPort->getType())))
		return false;

	// If they are input published ports and their types match, go ahead and merge them.
	if (this->getBase()->getInput())
		return true;

	// If they are output published ports and their types match, make sure they
	// have no more than one connected internal data port between the two of them.
	bool foundConnectedDataPort = false;
	set<VuoPort *> connectedPorts = getBase()->getConnectedPorts();
	for (set<VuoPort *>::iterator port = connectedPorts.begin(); (! foundConnectedDataPort) && (port != connectedPorts.end()); ++port)
	{
		if ((static_cast<VuoCompilerPortClass *>((*port)->getClass()->getCompiler()))->getDataVuoType())
			foundConnectedDataPort = true;
	}

	if (! foundConnectedDataPort)
		return true;

	else
	{
		set<VuoPort *> otherExternalPortConnectedPorts = otherExternalPort->getConnectedPorts();
		for (set<VuoPort *>::iterator port = otherExternalPortConnectedPorts.begin(); port != otherExternalPortConnectedPorts.end(); ++port)
		{
			if ((static_cast<VuoCompilerPortClass *>((*port)->getClass()->getCompiler()))->getDataVuoType())
				return false;
		}
	}

	return true;
}

/**
 * Returns the location of the published port within the published port sidebar,
 * in global coordinates.
 */
QPointF VuoRendererPublishedPort::getGlobalPos(void) const
{
	return globalPos;
}

/**
 * Sets the location of the published port within the published port sidebar,
 * in global coordinates.
 */
void VuoRendererPublishedPort::setGlobalPos(QPointF pos)
{
	this->globalPos = pos;
}
