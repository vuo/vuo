/**
 * @file
 * VuoRendererPublishedPort implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoPortClass.hh"
#include "VuoGenericType.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerOutputEventPortClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompilerCable.hh"
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
	this->compositionViewportPos = QPoint();
	this->isActive = false;

	// We can't use the VuoPseudoPort for rendering purposes because it won't necessarily have the type that we
	// want the rendering to reflect. (e.g., published input pseudoports are always event-only trigger ports.)
	// Create a new VuoRendererPort to represent this published port.
	this->portRepresentation = createPortRepresentation(getBase()->getName(), getBase()->getType(), getBase()->getInput());
	this->portRepresentation->setProxyPublishedSidebarPort(this);

	// The VuoPsuedoPort must nevertheless have its own associated VuoRendererPort -- not because it will
	// be rendered (it will not), but so that cables connected to it will have a way to access this VuoPublishedPort.
	VuoPort *pseudoPort = getBase()->getCompiler()->getVuoPseudoPort();
	pseudoPort->setRenderer(new VuoRendererPort(pseudoPort, NULL, getBase()->getInput(), false, false));
	pseudoPort->getRenderer()->setProxyPublishedSidebarPort(this);

	setFlag(QGraphicsItem::ItemIsSelectable, true);
	updateNameRect();
}

/**
 * Creates and returns a VuoRendererPort to represent a published port within the sidebar.
 *
 * @param name The name of the published port.
 * @param type The data type of the published port.
 * @param isPublishedInput A boolean indicating whether the published port is a published input, as
 * opposed to a published output.
 */
VuoRendererPort * VuoRendererPublishedPort::createPortRepresentation(string name, VuoType *type, bool isPublishedInput)
{
	VuoCompilerPort *portRepresentationCompiler;
	if (isPublishedInput)
	{
		VuoCompilerOutputEventPortClass *portRepresentationOutputCompilerClass = new VuoCompilerOutputEventPortClass(name);

		if (type)
		{
			portRepresentationOutputCompilerClass->setDataClass(new VuoCompilerOutputDataClass(name, NULL));
			portRepresentationOutputCompilerClass->setDataVuoType(type);
		}

		portRepresentationCompiler = portRepresentationOutputCompilerClass->newPort();
	}
	else
	{
		VuoCompilerInputEventPortClass *portRepresentationInputCompilerClass = new VuoCompilerInputEventPortClass(name);

		if (type)
		{
			portRepresentationInputCompilerClass->setDataClass(new VuoCompilerInputDataClass(name, NULL, false));
			portRepresentationInputCompilerClass->setDataVuoType(type);
		}
		portRepresentationCompiler = portRepresentationInputCompilerClass->newPort();
	}

	VuoRendererPort *portRepresentation = new VuoRendererPort(
		portRepresentationCompiler->getBase(),
		NULL,
		// "Published input" ports are *output* ports within the published input node,
		// and vice versa.
		isPublishedInput,
		false,
		false
		);

	return portRepresentation;
}

/**
 * Draws the published port on @c painter.
 */
void VuoRendererPublishedPort::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	portRepresentation->paint(painter, option, widget);
}

/**
 * Returns a rectangle containing the rendered published port.
 */
QRectF VuoRendererPublishedPort::boundingRect(void) const
{
	return portRepresentation->boundingRect();
}

/**
 * Sets the published port's name.
 */
void VuoRendererPublishedPort::setName(string name)
{
	getBase()->setName(name);
	updateNameRect();
}

/**
 * Returns a boolean indicating whether a new @c internalPort may be
 * attached to/from this externally visible published port without displacing
 * any currently connected internal data ports.
 *
 * If @c eventOnlyConnection is true, the potential connection will be evaluated as
 * an event-only connection, regardless of whether the connected ports themselves carry data.
 */
bool VuoRendererPublishedPort::canAccommodateInternalPort(VuoRendererPort *internalPort, bool eventOnlyConnection)
{
	if (! isCompatibleAliasWithSpecializationForInternalPort(internalPort, eventOnlyConnection))
		return false;

	// @todo https://b33p.net/kosada/node/5142 : Until a single cable connection can accomplish both,
	// port publication and unpublication, don't allow an internal-external port pair to be eligibility-highlighted
	// within a single cable drag even if their previous connected cable had a different data-carrying status.
	if (getBase()->getCompiler()->getVuoPseudoPort()->getCableConnecting(internalPort->getBase()))
		return false;

	// If this is a published output port that already has has a connected data-carrying cable,
	// it cannot accommodate another data-carrying cable.
	if (getBase()->getOutput() && !eventOnlyConnection)
	{
		foreach (VuoCable *connectedCable, getBase()->getCompiler()->getVuoPseudoPort()->getConnectedCables(true))
			if (connectedCable->getRenderer()->effectivelyCarriesData() &&
					connectedCable->getFromNode() &&
					connectedCable->getToNode())
			{
				return false;
			}
	}

	return true;
}

/**
 * Returns a boolean indicating whether a cable may be
 * attached directly between this externally visible published port
 * and the input @c internalPort, taking into account
 * the respective port types (input vs. output; event-only vs.
 * event+data; respective data types).
 *
 * If @c eventOnlyConnection is true, the potential connection will be evaluated as
 * an event-only connection, regardless of whether the connected ports themselves carry data.
 *
 * If the connection would require one or both ports to be specialized, returns false.
 * (But see @c VuoRendererPublishedPort::isCompatibleAliasWithSpecializationForInternalPort(...).)
 */
bool VuoRendererPublishedPort::isCompatibleAliasWithoutSpecializationForInternalPort(VuoRendererPort *internalPort, bool eventOnlyConnection)
{
	// Temporarily disallow direct cable connections between published inputs and published outputs.
	// @todo: Allow for https://b33p.net/kosada/node/7756 .
	if (internalPort->getProxyPublishedSidebarPort())
		return false;

	return (getBase()->getInput()? portRepresentation->canConnectDirectlyWithoutSpecializationTo(internalPort, eventOnlyConnection) :
								   internalPort->canConnectDirectlyWithoutSpecializationTo(portRepresentation, eventOnlyConnection));
}

/**
 * Returns a boolean indicating whether there may be a cable
 * attached directly between this externally visible published port
 * and the input @c internalPort, taking into account
 * the respective port types (input vs. output; event-only vs.
 * event+data; respective data types), and the possibility that one
 * port may be specialized in preparation for the connection.
 *
 * If @c eventOnlyConnection is true, the potential connection will be evaluated as
 * an event-only connection, regardless of whether the connected ports themselves carry data.
 *
 * Convenience function for VuoRendererPublishedPort::isCompatibleAliasWithSpecializationForInternalPort(VuoRendererPort *internalPort,
 * VuoRendererPort **portToSpecialize, string &specializedTypeName), for use
 * when only the returned boolean and none of the other output parameter values are needed.
 */
bool VuoRendererPublishedPort::isCompatibleAliasWithSpecializationForInternalPort(VuoRendererPort *internalPort, bool eventOnlyConnection)
{
	VuoRendererPort *portToSpecialize = NULL;
	string specializedTypeName = "";

	return this->isCompatibleAliasWithSpecializationForInternalPort(internalPort, eventOnlyConnection, &portToSpecialize, specializedTypeName);
}

/**
 * Returns a boolean indicating whether the provided @c cable may be
 * attached directly between this externally visible published port
 * and the input @c internalPort, taking into account
 * the respective port types (input vs. output; event-only vs.
 * event+data; respective data types), and the possibility that one
 * port may be specialized in preparation for the connection.
 *
 * If @c eventOnlyConnection is true, the potential connection will be evaluated as
 * an event-only connection, regardless of whether the connected ports themselves carry data.
 */
bool VuoRendererPublishedPort::isCompatibleAliasWithSpecializationForInternalPort(VuoRendererPort *internalPort,
																				  bool eventOnlyConnection,
																				  VuoRendererPort **portToSpecialize,
																				  string &specializedTypeName)
{
	*portToSpecialize = NULL;
	specializedTypeName = "";

	if (this->isCompatibleAliasWithoutSpecializationForInternalPort(internalPort, eventOnlyConnection))
		return true;

	// Temporarily disallow direct cable connections between published inputs and published outputs.
	// @todo: Allow for https://b33p.net/kosada/node/7756 .
	if (internalPort->getProxyPublishedSidebarPort())
		return false;

	return (getBase()->getInput()? portRepresentation->canConnectDirectlyWithSpecializationTo(internalPort, eventOnlyConnection, portToSpecialize, specializedTypeName) :
								   internalPort->canConnectDirectlyWithSpecializationTo(portRepresentation, eventOnlyConnection, portToSpecialize, specializedTypeName));
}

/**
 * Returns a boolean indicating whether the @c otherExternalPort may be
 * merged with this one, taking into account
 * the respective port types (input vs. output; event-only vs.
 * event+data; respective data types), without
 * displacing any currently connected internal data ports.
 * The @c mergeWillAddData input should indicate whether
 * the @c otherExternalPort is expected to have connected data sources.
 */
bool VuoRendererPublishedPort::canBeMergedWith(VuoPublishedPort *otherExternalPort, bool mergeWillAddData)
{
	// For these two externally visible published ports to be eligible for merging, they must have the same type.
	if (! ((this->getBase()->getInput() == otherExternalPort->getInput()) &&
		   (this->getBase()->getType() == otherExternalPort->getType())))
		return false;

	// If they are input published ports and their types match, go ahead and merge them.
	if (this->getBase()->getInput())
		return true;

	// If they are output published ports and their types match, make sure they will
	// have no more than one connected data-carrying cable between the two of them.
	if (!mergeWillAddData)
		return true;

	foreach (VuoCable *connectedCable, getBase()->getCompiler()->getVuoPseudoPort()->getConnectedCables(true))
	{
		if (connectedCable->getRenderer()->effectivelyCarriesData())
			return false;
	}

	return true;
}

/**
 * Adds the specified @c port to the list of internal ports for which this published port is an alias.
 */
void VuoRendererPublishedPort::addConnectedPort(VuoPort *port)
{
	getBase()->addConnectedPort(port);
	updateNameRect();
}

/**
 * Removes the specified @c port from the list of internal ports for which this published port is an alias.
 */
void VuoRendererPublishedPort::removeConnectedPort(VuoPort *port)
{
	getBase()->removeConnectedPort(port);
	updateNameRect();
}

/**
 * Returns the location of the rendered sidebar published port,
 * in coordinates relative to the composition viewport.
 */
QPoint VuoRendererPublishedPort::getCompositionViewportPos(void) const
{
	return compositionViewportPos;
}

/**
 * Sets the location of the rendered sidebar published port,
 * in coordinates relative to the composition viewport.
 */
void VuoRendererPublishedPort::setCompositionViewportPos(QPoint pos)
{
	this->compositionViewportPos = pos;
}

/**
 * Returns the VuoRendererPort that visually represents this published port.
 */
VuoRendererPort * VuoRendererPublishedPort::getPortRepresentation()
{
	return portRepresentation;
}

/**
 * Updates the cached bounding box of the published port's label within the published port sidebar.
 */
void VuoRendererPublishedPort::updateNameRect()
{
	portRepresentation->updateNameRect();
}

/**
 * Sets a boolean indicating whether this published port will be painted against an active
 * item background within the published port sidebar.
 */
void VuoRendererPublishedPort::setCurrentlyActive(bool active)
{
	this->isActive = active;
}

/**
 * Returns the boolean indicating whether this published port will be painted against an active
 * item background within the published port sidebar.
 */
bool VuoRendererPublishedPort::getCurrentlyActive()
{
	return this->isActive;
}
