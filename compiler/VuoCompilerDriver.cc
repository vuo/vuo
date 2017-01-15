/**
 * @file
 * VuoCompilerDriver implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerDriver.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerGraphvizParser.hh"
#include "VuoCompilerPort.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoComposition.hh"
#include "VuoNode.hh"
#include "VuoPort.hh"
#include "VuoProtocol.hh"
#include "VuoPublishedPort.hh"
#include "VuoType.hh"

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
	vector<VuoPublishedPort *> driverBridgeInputs = parser->getPublishedOutputPorts();
	for (vector<VuoPublishedPort *>::iterator i = driverBridgeInputs.begin(); i != driverBridgeInputs.end(); ++i)
	{
		VuoPublishedPort *driverBridgeInput = (*i);
		string driverBridgeInputName = driverBridgeInput->getClass()->getName();
		VuoType *driverBridgeInputType = static_cast<VuoCompilerPublishedPort *>(driverBridgeInput->getCompiler())->getDataVuoType();

		if (!protocol->hasInputPort(driverBridgeInputName) ||
				(protocol->getTypeForInputPort(driverBridgeInputName) != driverBridgeInputType->getModuleKey()))
		{
			return false;
		}
	}

	// For each published input port in driver, make sure the protocol has a published
	// output port with a matching name and type.
	vector<VuoPublishedPort *> driverBridgeOutputs = parser->getPublishedInputPorts();
	for (vector<VuoPublishedPort *>::iterator i = driverBridgeOutputs.begin(); i != driverBridgeOutputs.end(); ++i)
	{
		VuoPublishedPort *driverBridgeOutput = (*i);
		string driverBridgeOutputName = driverBridgeOutput->getClass()->getName();
		VuoType *driverBridgeOutputType = static_cast<VuoCompilerPublishedPort *>(driverBridgeOutput->getCompiler())->getDataVuoType();

		if (!protocol->hasOutputPort(driverBridgeOutputName) ||
				(protocol->getTypeForOutputPort(driverBridgeOutputName) != driverBridgeOutputType->getModuleKey()))
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
	vector<VuoPublishedPort *> driverBridgeInputs = parser->getPublishedOutputPorts();
	for (vector<VuoPublishedPort *>::iterator i = driverBridgeInputs.begin(); i != driverBridgeInputs.end(); ++i)
	{
		VuoPublishedPort *driverBridgeInput = (*i);
		string driverBridgeInputName = driverBridgeInput->getClass()->getName();
		VuoType *driverBridgeInputType = static_cast<VuoCompilerPublishedPort *>(driverBridgeInput->getCompiler())->getDataVuoType();

		VuoPublishedPort *compositionPublishedInput = composition->getBase()->getPublishedInputPortWithName(driverBridgeInputName);
		if (compositionPublishedInput)
		{
			VuoType *compositionPublishedInputType = static_cast<VuoCompilerPublishedPort *>(compositionPublishedInput->getCompiler())->getDataVuoType();
			if (compositionPublishedInputType == driverBridgeInputType)
			{
				vector<VuoCable *> publishedInputOutgoingCables = compositionPublishedInput->getConnectedCables();
				set<pair<VuoPort *, VuoNode *> > compositionConnectedPorts;
				map<pair<VuoPort *, VuoNode *>, bool> eventOnlyConnection;

				// Re-route the driver published output's incoming cables to each of the internal ports connected
				// to the composition's corresponding published input.
				for (vector<VuoCable *>::iterator i = publishedInputOutgoingCables.begin(); i != publishedInputOutgoingCables.end(); ++i)
				{
					VuoCable *cable = (*i);
					VuoPort *toPort = cable->getToPort();
					VuoNode *toNode = cable->getToNode();
					compositionConnectedPorts.insert(make_pair(toPort, toNode));
					eventOnlyConnection[make_pair(toPort, toNode)] = cable->getCompiler()->getAlwaysEventOnly();

					// Remove the original outgoing cables from the composition's published input.
					composition->getBase()->removeCable(cable);
				}

				// Remove the composition's published input.
				int index = composition->getBase()->getIndexOfPublishedPort(compositionPublishedInput, true);
				if (index != -1)
					composition->getBase()->removePublishedInputPort(index);

				vector<VuoCable *> driverBridgeInputIncomingCables = driverBridgeInput->getConnectedCables();
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
						if (cable->getCompiler()->getAlwaysEventOnly() || eventOnlyConnection[(*i)])
							newCable->setAlwaysEventOnly(true);

						composition->getBase()->addCable(newCable->getBase());
					}
				}
			}
		}
	}

	// Bridge each of the driver's published inputs with the matching composition published output.
	vector<VuoPublishedPort *> driverBridgeOutputs = parser->getPublishedInputPorts();
	for (vector<VuoPublishedPort *>::iterator i = driverBridgeOutputs.begin(); i != driverBridgeOutputs.end(); ++i)
	{
		VuoPublishedPort *driverBridgeOutput = (*i);
		string driverBridgeOutputName = driverBridgeOutput->getClass()->getName();
		VuoType *driverBridgeOutputType = static_cast<VuoCompilerPublishedPort *>(driverBridgeOutput->getCompiler())->getDataVuoType();

		VuoPublishedPort *compositionPublishedOutput = composition->getBase()->getPublishedOutputPortWithName(driverBridgeOutputName);
		if (compositionPublishedOutput)
		{
			VuoType *compositionPublishedOutputType = static_cast<VuoCompilerPublishedPort *>(compositionPublishedOutput->getCompiler())->getDataVuoType();
			if (compositionPublishedOutputType == driverBridgeOutputType)
			{
				vector<VuoCable *> publishedOutputIncomingCables = compositionPublishedOutput->getConnectedCables();
				set<pair<VuoPort *, VuoNode *> > compositionConnectedPorts;
				map<pair<VuoPort *, VuoNode *>, bool> eventOnlyConnection;

				// Re-route the composition published output's incoming cables to each of the internal ports connected
				// to the driver's corresponding published input.
				for (vector<VuoCable *>::iterator i = publishedOutputIncomingCables.begin(); i != publishedOutputIncomingCables.end(); ++i)
				{
					VuoCable *cable = (*i);
					VuoPort *fromPort = cable->getFromPort();
					VuoNode *fromNode = cable->getFromNode();
					compositionConnectedPorts.insert(make_pair(fromPort, fromNode));
					eventOnlyConnection[make_pair(fromPort, fromNode)] = cable->getCompiler()->getAlwaysEventOnly();

					// Remove the original outgoing cables from the composition's published input.
					composition->getBase()->removeCable(cable);
				}

				// Remove the composition's published output.
				int index = composition->getBase()->getIndexOfPublishedPort(compositionPublishedOutput, false);
				if (index != -1)
					composition->getBase()->removePublishedOutputPort(index);

				vector<VuoCable *> driverBridgeOutputOutgoingCables = driverBridgeOutput->getConnectedCables();
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
						if (cable->getCompiler()->getAlwaysEventOnly() || eventOnlyConnection[(*i)])
							newCable->setAlwaysEventOnly(true);

						composition->getBase()->addCable(newCable->getBase());
					}
				}
			}
		}
	}

	// Add an event-only cable from the driver's first published output to each of the composition's non-protocol
	// published inputs.
	if (! driverBridgeInputs.empty())
	{
		VuoPublishedPort *driverBridgeInput = driverBridgeInputs[0];
		vector<VuoCable *> driverBridgeInputIncomingCables = driverBridgeInput->getConnectedCables();

		vector<VuoPublishedPort *> nonProtocolPublishedInputs = composition->getBase()->getPublishedInputPorts();
		for (vector<VuoPublishedPort *>::iterator i = nonProtocolPublishedInputs.begin(); i != nonProtocolPublishedInputs.end(); ++i)
		{
			VuoPublishedPort *compositionPublishedInput = *i;

			vector<VuoCable *> publishedInputOutgoingCables = compositionPublishedInput->getConnectedCables();
			for (vector<VuoCable *>::iterator i = publishedInputOutgoingCables.begin(); i != publishedInputOutgoingCables.end(); ++i)
			{
				VuoCable *compositionCable = (*i);
				VuoPort *toPort = compositionCable->getToPort();
				VuoNode *toNode = compositionCable->getToNode();

				for (vector<VuoCable *>::iterator j = driverBridgeInputIncomingCables.begin(); j != driverBridgeInputIncomingCables.end(); ++j)
				{
					VuoCable *driverCable = (*j);
					VuoPort *fromPort = driverCable->getFromPort();
					VuoNode *fromNode = driverCable->getFromNode();

					VuoCompilerPort *fromCompilerPort = static_cast<VuoCompilerPort *>(fromPort->getCompiler());
					VuoCompilerPort *toCompilerPort = static_cast<VuoCompilerPort *>(toPort->getCompiler());
					VuoCompilerCable *newCable = new VuoCompilerCable(fromNode->getCompiler(), fromCompilerPort, toNode->getCompiler(), toCompilerPort);
					newCable->setAlwaysEventOnly(true);

					composition->getBase()->addCable(newCable->getBase());
				}
			}
		}
	}
}
