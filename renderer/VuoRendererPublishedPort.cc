/**
 * @file
 * VuoRendererPublishedPort implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoPortClass.hh"
#include "VuoGenericType.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerOutputDataClass.hh"
#include "VuoCompilerOutputEventPortClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerPort.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompilerCable.hh"
#include "VuoRendererPublishedPort.hh"
#include "VuoRendererPort.hh"

/**
 * Creates a renderer detail for the specified base published port.
 */
VuoRendererPublishedPort::VuoRendererPublishedPort(VuoPublishedPort *basePublishedPort, bool isPublishedOutput)
	: VuoRendererPort(basePublishedPort,
					  NULL,
					  // "Published input" ports are *output* ports within the published input node,
					  // and vice versa.
					  !isPublishedOutput,
					  false,
					  false
					  )
{
	this->compositionViewportPos = QPoint();
	this->isActive = false;

	setFlag(QGraphicsItem::ItemIsSelectable, true);
	updateNameRect();
}

/**
 * Sets the published port's name.
 */
void VuoRendererPublishedPort::setName(string name)
{
	getBase()->getClass()->setName(name);
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
	if (getBase()->getCableConnecting(internalPort->getBase()))
		return false;

	// If this is a published output port that already has has a connected data-carrying cable,
	// it cannot accommodate another data-carrying cable.
	bool isPublishedOutput = getInput();
	if (isPublishedOutput && !eventOnlyConnection)
	{
		foreach (VuoCable *connectedCable, getBase()->getConnectedCables(true))
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
	if (dynamic_cast<VuoRendererPublishedPort *>(internalPort))
		return false;

	bool isPublishedInput = !getInput();
	return (isPublishedInput? canConnectDirectlyWithoutSpecializationTo(internalPort, eventOnlyConnection) :
							  internalPort->canConnectDirectlyWithoutSpecializationTo(this, eventOnlyConnection));
	return false;
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
	if (dynamic_cast<VuoRendererPublishedPort *>(internalPort))
		return false;

	bool isPublishedInput = !getInput();
	return (isPublishedInput? canConnectDirectlyWithSpecializationTo(internalPort, eventOnlyConnection, portToSpecialize, specializedTypeName) :
								   internalPort->canConnectDirectlyWithSpecializationTo(this, eventOnlyConnection, portToSpecialize, specializedTypeName));
	return false;
}

/**
 * Returns a boolean indicating whether the @c otherExternalPort may be
 * merged with this one, taking into account
 * the respective port types (event-only vs.
 * event+data; respective data types), without
 * displacing any currently connected internal data ports.
 * The @c mergeWillAddData input should indicate whether
 * the @c otherExternalPort is expected to have connected data sources.
 *
 * Assumes that the provided @c externalOtherPort is of the same input/output
 * type as this port, since published ports of mismatched input/output types
 * would never be merged.
 */
bool VuoRendererPublishedPort::canBeMergedWith(VuoPublishedPort *otherExternalPort, bool mergeWillAddData)
{
	bool thisIsPublishedInput = !getInput();

	// For these two externally visible published ports to be eligible for merging, they must have the same type.
	if (!otherExternalPort->hasCompiler() ||
			(static_cast<VuoCompilerPort *>(otherExternalPort->getCompiler())->getDataVuoType() != this->getDataType()))
		return false;

	// If they are input published ports and their types match, go ahead and merge them.
	if (thisIsPublishedInput)
		return true;

	// If they are output published ports and their types match, make sure they will
	// have no more than one connected data-carrying cable between the two of them.
	if (!mergeWillAddData)
		return true;

	foreach (VuoCable *connectedCable, getBase()->getConnectedCables(true))
	{
		if (connectedCable->getRenderer()->effectivelyCarriesData())
			return false;
	}

	return true;
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

/**
 * Returns the path of the antenna that represents any connected wireless cables, or an empty path if not applicable.
 */
QPainterPath VuoRendererPublishedPort::getWirelessAntennaPath() const
{
	return QPainterPath();
}
