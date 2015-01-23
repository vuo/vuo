/**
 * @file
 * VuoCompilerCable implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>

#include "VuoCompilerCable.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerOutputEventPort.hh"
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

	if (fromNode && fromNode->hasCompiler() && toNode && toNode->hasCompiler() && fromPort && toPort)
	{
		declaration << fromNode->getCompiler()->getGraphvizIdentifier() << ":" << fromPort->getClass()->getName() << " -> "
					<< toNode->getCompiler()->getGraphvizIdentifier() << ":" << toPort->getClass()->getName() << ";";
	}

	return declaration.str();
}

/**
 * Returns a boolean indicating whether this cable carries data.
 */
bool VuoCompilerCable::carriesData(void)
{
	VuoNode *fromNode = getBase()->getFromNode();
	VuoNode *toNode = getBase()->getToNode();
	VuoPort *fromPort = getBase()->getFromPort();
	VuoPort *toPort = getBase()->getToPort();

	bool fromPortHasData =	fromPort &&
							fromPort->getClass()->hasCompiler() &&
							static_cast<VuoCompilerPortClass *>(fromPort->getClass()->getCompiler())->getDataVuoType();

	bool toPortHasData =	toPort &&
							toPort->getClass()->hasCompiler() &&
							static_cast<VuoCompilerPortClass *>(toPort->getClass()->getCompiler())->getDataVuoType();

	// If not currently connected to a 'From' port, decide on the basis of the 'To' port alone.
	if (! fromPort)
		return toPortHasData;

	// If not currently connected to a 'To' port, decide on the basis of the 'From' port alone.
	else if (! toPort)
		return fromPortHasData;

	// If connected at both ends, but the 'From' port is a published input port (guaranteed not to technically carry data),
	// decide on the basis of the 'To' port alone.
	// @todo: Instead take into account the inherent type of the published input port.
	else if (fromNode && fromNode->getNodeClass()->getClassName() == VuoNodeClass::publishedInputNodeClassName)
		return toPortHasData;

	// If connected at both ends, but the 'To' port is a published output port (guaranteed not to technically carry data),
	// decide on the basis of the 'From' port alone.
	// @todo: Instead take into account the inherent type of the published output port.
	else if (toNode && toNode->getNodeClass()->getClassName() == VuoNodeClass::publishedOutputNodeClassName)
		return fromPortHasData;

	// If connected at both ends and neither connected port is a published port, the cable carries
	// data if and only if each of its connected ports contain data.
	else
		return (fromPortHasData && toPortHasData);
}

