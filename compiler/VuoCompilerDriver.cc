/**
 * @file
 * VuoCompilerDriver implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerDriver.hh"
#include "VuoCompilerCable.hh"
#include "VuoComposition.hh"
#include "VuoPort.hh"

/**
 * Creates a driver.
 *
 * @param compiler The compiler to use for looking up node classes.
 * @param driverAsCompositionString The serialized driver, in the form of .vuo composition Graphviz source.
 */
VuoCompilerDriver::VuoCompilerDriver(VuoCompiler *compiler, const string &driverAsCompositionString)
{
	this->parser = VuoCompilerGraphvizParser::newParserFromCompositionString(driverAsCompositionString, compiler);
}

/**
 * Returns a boolean indicating whether this driver is valid for the provided @c protocol.
 * A driver is valid for a given protocol if, for each of the driver's published outputs, the protocol
 * has a published input with the same name and type, and for each of the driver's published inputs,
 * the protocol has a published output with the same name and type.  The protocol may have
 * additional published inputs and outputs and still be compatible with the driver.
 */
bool VuoCompilerDriver::isValidDriverForProtocol(VuoProtocol *protocol)
{
	// For each published output port in driver, make sure the protocol has a published
	// input port with a matching name and type.
	vector<VuoCompilerPublishedPort *> driverBridgeInputs = parser->getPublishedOutputPorts();
	for (vector<VuoCompilerPublishedPort *>::iterator i = driverBridgeInputs.begin(); i != driverBridgeInputs.end(); ++i)
	{
		VuoCompilerPublishedPort *driverBridgeInput = (*i);
		string driverBridgeInputName = driverBridgeInput->getBase()->getName();
		VuoType *driverBridgeInputType = driverBridgeInput->getBase()->getType();

		if (!protocol->hasInputPort(driverBridgeInputName) ||
				(protocol->getTypeForInputPort(driverBridgeInputName) != driverBridgeInputType))
		{
			return false;
		}
	}

	// For each published input port in driver, make sure the protocol has a published
	// output port with a matching name and type.
	vector<VuoCompilerPublishedPort *> driverBridgeOutputs = parser->getPublishedInputPorts();
	for (vector<VuoCompilerPublishedPort *>::iterator i = driverBridgeOutputs.begin(); i != driverBridgeOutputs.end(); ++i)
	{
		VuoCompilerPublishedPort *driverBridgeOutput = (*i);
		string driverBridgeOutputName = driverBridgeOutput->getBase()->getName();
		VuoType *driverBridgeOutputType = driverBridgeOutput->getBase()->getType();

		if (!protocol->hasOutputPort(driverBridgeOutputName) ||
				(protocol->getTypeForOutputPort(driverBridgeOutputName) != driverBridgeOutputType))
		{
			return false;
		}
	}

	return true;
}

/**
 * Applies this driver to the provided @c composition.
 *
 * Bridges the driver with the composition by matching published output ports
 * within the driver to identically named and typed published input ports within the
 * composition, and vice versa, and re-routing their connected cables appropriately.
 */
void VuoCompilerDriver::applyToComposition(VuoCompilerComposition *composition)
{
	// Add the (non-published) driver nodes to the composition.
	vector<VuoNode *> nodes = parser->getNodes();
	for (vector<VuoNode *>::iterator node = nodes.begin(); node != nodes.end(); ++node)
	{
		composition->getBase()->addNode(*node);
		composition->setUniqueGraphvizIdentifierForNode(*node);
	}

	// Add the (non-published) driver cables to the composition.
	vector<VuoCable *> cables = parser->getCables();
	for (vector<VuoCable *>::iterator cable = cables.begin(); cable != cables.end(); ++cable)
		composition->getBase()->addCable(*cable);

	// Bridge each of the driver's published outputs with the matching composition published input.
	vector<VuoCompilerPublishedPort *> driverBridgeInputs = parser->getPublishedOutputPorts();
	for (vector<VuoCompilerPublishedPort *>::iterator i = driverBridgeInputs.begin(); i != driverBridgeInputs.end(); ++i)
	{
		VuoCompilerPublishedPort *driverBridgeInput = (*i);
		string driverBridgeInputName = driverBridgeInput->getBase()->getName();
		VuoType *driverBridgeInputType = driverBridgeInput->getBase()->getType();

		VuoPublishedPort *compositionPublishedInput = composition->getBase()->getPublishedInputPortWithName(driverBridgeInputName);
		if (compositionPublishedInput && (compositionPublishedInput->getType() == driverBridgeInputType))
		{
			VuoPort *compositionPseudoPort = compositionPublishedInput->getCompiler()->getVuoPseudoPort();
			vector<VuoCable *> publishedInputOutgoingCables = compositionPseudoPort->getConnectedCables(true);
			set<pair<VuoPort *, VuoNode *> > compositionConnectedPorts;

			// Re-route the driver published output's incoming cables to each of the internal ports connected
			// to the composition's corresponding published input.
			// @todo: Once cables have inherent types, accurately preserve/determine those types when re-routing
			// these connections. https://b33p.net/kosada/node/6055
			for (vector<VuoCable *>::iterator i = publishedInputOutgoingCables.begin(); i != publishedInputOutgoingCables.end(); ++i)
			{
				VuoCable *cable = (*i);
				VuoPort *toPort = cable->getToPort();
				VuoNode *toNode = cable->getToNode();
				compositionConnectedPorts.insert(make_pair(toPort, toNode));

				// Remove the original outgoing cables from the composition's published input.
				composition->getBase()->removePublishedInputCable(cable);
				compositionPublishedInput->removeConnectedPort(toPort);
			}

			// Remove the composition's published input.
			int index = composition->getBase()->getIndexOfPublishedPort(compositionPublishedInput, true);
			if (index != -1)
				composition->getBase()->removePublishedInputPort(index);

			VuoPort *driverPseudoPort = driverBridgeInput->getVuoPseudoPort();
			vector<VuoCable *> driverBridgeInputIncomingCables = driverPseudoPort->getConnectedCables(true);
			for (vector<VuoCable *>::iterator i = driverBridgeInputIncomingCables.begin(); i != driverBridgeInputIncomingCables.end(); ++i)
			{
				VuoCable *cable = (*i);
				VuoPort *fromPort = cable->getFromPort();
				VuoNode *fromNode = cable->getFromNode();

				// For each of the driver published output's incoming cables, create new cables connecting
				// that cable's 'From' port to each of the composition's internal ports originally connected
				// to the composition published input.
				for (set<pair<VuoPort *, VuoNode *> >::iterator i = compositionConnectedPorts.begin(); i != compositionConnectedPorts.end(); ++i)
				{
					VuoPort *toPort = i->first;
					VuoNode *toNode = i->second;

					VuoCompilerPort *fromCompilerPort = static_cast<VuoCompilerPort *>(fromPort->getCompiler());
					VuoCompilerPort *toCompilerPort = static_cast<VuoCompilerPort *>(toPort->getCompiler());
					VuoCompilerCable *newCable = new VuoCompilerCable(fromNode->getCompiler(), fromCompilerPort, toNode->getCompiler(), toCompilerPort);
					composition->getBase()->addCable(newCable->getBase());
				}
			}
		}
	}

	// Bridge each of the driver's published inputs with the matching composition published output.
	vector<VuoCompilerPublishedPort *> driverBridgeOutputs = parser->getPublishedInputPorts();
	for (vector<VuoCompilerPublishedPort *>::iterator i = driverBridgeOutputs.begin(); i != driverBridgeOutputs.end(); ++i)
	{
		VuoCompilerPublishedPort *driverBridgeOutput = (*i);
		string driverBridgeOutputName = driverBridgeOutput->getBase()->getName();
		VuoType *driverBridgeOutputType = driverBridgeOutput->getBase()->getType();

		VuoPublishedPort *compositionPublishedOutput = composition->getBase()->getPublishedOutputPortWithName(driverBridgeOutputName);
		if (compositionPublishedOutput && (compositionPublishedOutput->getType() == driverBridgeOutputType))
		{
			VuoPort *compositionPseudoPort = compositionPublishedOutput->getCompiler()->getVuoPseudoPort();
			vector<VuoCable *> publishedOutputIncomingCables = compositionPseudoPort->getConnectedCables(true);
			set<pair<VuoPort *, VuoNode *> > compositionConnectedPorts;

			// Re-route the composition published output's incoming cables to each of the internal ports connected
			// to the driver's corresponding published input.
			// @todo: Once cables have inherent types, accurately preserve/determine those types when re-routing
			// these connections. https://b33p.net/kosada/node/6055
			for (vector<VuoCable *>::iterator i = publishedOutputIncomingCables.begin(); i != publishedOutputIncomingCables.end(); ++i)
			{
				VuoCable *cable = (*i);
				VuoPort *fromPort = cable->getFromPort();
				VuoNode *fromNode = cable->getFromNode();
				compositionConnectedPorts.insert(make_pair(fromPort, fromNode));

				// Remove the original outgoing cables from the composition's published input.
				composition->getBase()->removePublishedOutputCable(cable);
				compositionPublishedOutput->removeConnectedPort(fromPort);
			}

			// Remove the composition's published output.
			int index = composition->getBase()->getIndexOfPublishedPort(compositionPublishedOutput, false);
			if (index != -1)
				composition->getBase()->removePublishedOutputPort(index);

			VuoPort *driverPseudoPort = driverBridgeOutput->getVuoPseudoPort();
			vector<VuoCable *> driverBridgeOutputOutgoingCables = driverPseudoPort->getConnectedCables(true);
			for (vector<VuoCable *>::iterator i = driverBridgeOutputOutgoingCables.begin(); i != driverBridgeOutputOutgoingCables.end(); ++i)
			{
				VuoCable *cable = (*i);
				VuoPort *toPort = cable->getToPort();
				VuoNode *toNode = cable->getToNode();

				// For each of the driver published input's outgoing cables, create new cables connecting
				// that cable's 'To' port to each of the composition's internal ports originally connected
				// to the composition published output.
				for (set<pair<VuoPort *, VuoNode *> >::iterator i = compositionConnectedPorts.begin(); i != compositionConnectedPorts.end(); ++i)
				{
					VuoPort *fromPort = i->first;
					VuoNode *fromNode = i->second;

					VuoCompilerPort *fromCompilerPort = static_cast<VuoCompilerPort *>(fromPort->getCompiler());
					VuoCompilerPort *toCompilerPort = static_cast<VuoCompilerPort *>(toPort->getCompiler());
					VuoCompilerCable *newCable = new VuoCompilerCable(fromNode->getCompiler(), fromCompilerPort, toNode->getCompiler(), toCompilerPort);
					composition->getBase()->addCable(newCable->getBase());
				}
			}
		}
	}
}
