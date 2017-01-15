/**
 * @file
 * VuoCompilerCable implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>

#include "VuoCompilerCable.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerDataClass.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerOutputEventPort.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerType.hh"
#include "VuoNode.hh"
#include "VuoNodeClass.hh"
#include "VuoPort.hh"

/**
 * Creates a cable from @c fromNode's @c fromPort to @c toNode's @c toPort.
 */
VuoCompilerCable::VuoCompilerCable(VuoCompilerNode * fromNode, VuoCompilerPort * fromPort, VuoCompilerNode * toNode, VuoCompilerPort * toPort)
	: VuoBaseDetail<VuoCable>("VuoCompilerCable",
								new VuoCable(	fromNode? fromNode->getBase() : NULL,
												fromPort? fromPort->getBase() : NULL,
												toNode? toNode->getBase() : NULL,
												toPort? toPort->getBase() : NULL))
{
	getBase()->setCompiler(this);
	isAlwaysEventOnly = false;
	isHidden = false;
}

/**
 * Sets whether this cable is always event-only. If true, the cable is always event-only, even if its from-port and to-port
 * can both carry data. If false, the cable is event-only only if its from-port and/or to-port is event-only.
 */
void VuoCompilerCable::setAlwaysEventOnly(bool isEventOnly)
{
	this->isAlwaysEventOnly = isEventOnly;
}

/**
 * Returns a boolean indicating whether this cable is always event-only, regardless of the data-carrying status
 * of its from-port and to-port.
 */
bool VuoCompilerCable::getAlwaysEventOnly()
{
	return this->isAlwaysEventOnly;
}

/**
 * Sets whether this cable is a hidden ("wireless") cable.
 */
void VuoCompilerCable::setHidden(bool hidden)
{
	this->isHidden = hidden;
}

/**
 * Returns a boolean indicating whether this cable is a hidden ("wireless") cable.
 */
bool VuoCompilerCable::getHidden()
{
	return this->isHidden;
}

/**
 * Returns true if the given cable endpoint can carry data.
 */
bool VuoCompilerCable::portHasData(VuoPort *port)
{
	return (port &&
			port->getClass()->hasCompiler() &&
			static_cast<VuoCompilerPortClass *>(port->getClass()->getCompiler())->getDataVuoType());
}

/**
 * Returns a string containing the declaration for this cable
 * as it would appear in a .vuo (Graphviz dot format) file.
 */
string VuoCompilerCable::getGraphvizDeclaration(void)
{
	ostringstream declaration;

	VuoNode *fromNode = getBase()->getFromNode();
	VuoPort *fromPort = getBase()->getFromPort();
	VuoNode *toNode = getBase()->getToNode();
	VuoPort *toPort = getBase()->getToPort();

	if (fromNode && (fromNode->hasCompiler() || getBase()->isPublishedInputCable())
			&& toNode && (toNode->hasCompiler() || getBase()->isPublishedOutputCable())
			&& fromPort && toPort)
	{
		string fromNodeIdentifier = (fromNode && fromNode->hasCompiler() ?
										 fromNode->getCompiler()->getGraphvizIdentifier() :
										 VuoNodeClass::publishedInputNodeIdentifier);
		string toNodeIdentifier = (toNode && toNode->hasCompiler() ?
									   toNode->getCompiler()->getGraphvizIdentifier() :
									   VuoNodeClass::publishedOutputNodeIdentifier);

		declaration << fromNodeIdentifier << ":" << fromPort->getClass()->getName() << " -> "
					<< toNodeIdentifier << ":" << toPort->getClass()->getName();

		// Cable attributes
		bool isExplicitlyEventOnly = (isAlwaysEventOnly && portHasData( getBase()->getFromPort() ) && portHasData( getBase()->getToPort() ));
		if (isExplicitlyEventOnly || isHidden)
		{
			declaration << " [";

			if (isExplicitlyEventOnly)
				declaration << "event=true";

			if (isExplicitlyEventOnly && isHidden)
				declaration << " ";

			if (isHidden)
				declaration << "style=invis";

			declaration << "]";
		}

		declaration << ";";
	}

	return declaration.str();
}

/**
 * Returns a boolean indicating whether this cable carries data.
 */
bool VuoCompilerCable::carriesData(void)
{
	if (isAlwaysEventOnly)
		return false;

	VuoPort *fromPort = getBase()->getFromPort();
	VuoPort *toPort = getBase()->getToPort();

	bool fromPortHasData = portHasData(fromPort);
	bool toPortHasData = portHasData(toPort);

	// If not currently connected to a 'From' port, decide on the basis of the 'To' port alone.
	if (! fromPort)
		return toPortHasData;

	// If not currently connected to a 'To' port, decide on the basis of the 'From' port alone.
	else if (! toPort)
		return fromPortHasData;

	// If connected at both ends, the cable carries data if and only if each of its connected ports contain data.
	else
		return (fromPortHasData && toPortHasData);
}

/**
 * Generates code to transmit the data (if any) and an event (if any) from an output port to an input port.
 */
void VuoCompilerCable::generateTransmission(Module *module, BasicBlock *block, Value *toNodeContextValue, Value *toPortContextValue,
											Value *outputDataValue, bool shouldTransmitEvent)
{
	VuoCompilerInputEventPort *inputEventPort = static_cast<VuoCompilerInputEventPort *>(getBase()->getToPort()->getCompiler());

	if (outputDataValue)
		inputEventPort->generateReplaceData(module, block, toNodeContextValue, outputDataValue, toPortContextValue);

	if (shouldTransmitEvent)
		inputEventPort->generateStoreEvent(module, block, toNodeContextValue, true, toPortContextValue);
}
