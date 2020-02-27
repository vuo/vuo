/**
 * @file
 * VuoCompilerDriver implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCable.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerDriver.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerGraphvizParser.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerType.hh"
#include "VuoComposition.hh"
#include "VuoNode.hh"
#include "VuoNodeClass.hh"
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
	this->compiler = compiler;
	this->parser = VuoCompilerGraphvizParser::newParserFromCompositionString(driverAsCompositionString, compiler);
	this->compositionString = driverAsCompositionString;
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
void VuoCompilerDriver::applyToComposition(VuoCompilerComposition *composition, VuoCompiler *compiler, bool canPublishedInputsBeEdited)
{
	// Copy the driver's contents so the original driver won't be affected.
	VuoCompilerGraphvizParser *parserCopy = VuoCompilerGraphvizParser::newParserFromCompositionString(compositionString, compiler);

	// Add the (non-published) driver nodes to the composition.
	vector<VuoNode *> nodes = parserCopy->getNodes();
	for (vector<VuoNode *>::iterator node = nodes.begin(); node != nodes.end(); ++node)
	{
		// Give the node an identifier that is different from any nodes in the current composition
		// and any `Make List` nodes that have just been replaced during a live-coding reload.
		if ((*node)->hasCompiler())
		{
			string nodeIdentifierPrefix = (*node)->getCompiler()->getGraphvizIdentifierPrefix() + "_Driver";
			composition->setUniqueGraphvizIdentifierForNode(*node, nodeIdentifierPrefix, nodeIdentifierPrefix);
		}

		composition->getBase()->addNode(*node);
	}

	// Add the (non-published) driver cables to the composition.
	vector<VuoCable *> cables = parserCopy->getCables();
	for (vector<VuoCable *>::iterator cable = cables.begin(); cable != cables.end(); ++cable)
		if (! (*cable)->isPublished())
			composition->getBase()->addCable(*cable);

	// Bridge each of the driver's published outputs with the matching composition published input.
	vector<VuoPublishedPort *> driverBridgeInputs = parserCopy->getPublishedOutputPorts();
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
	vector<VuoPublishedPort *> driverBridgeOutputs = parserCopy->getPublishedInputPorts();
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

	// For each data published input in the composition that does not have a corresponding published output in the driver,
	// add an `Allow Changes` or `Allow First Value` node with an incoming data cable from the published input port,
	// an incoming event cable from the driver's `time` published output port, and an outgoing event cable to each
	// internal input port connected to the published input port.
	auto driverBridgeInput = std::find_if(driverBridgeInputs.begin(), driverBridgeInputs.end(),
										  [] (VuoPublishedPort *p) { return p->getClass()->getName() == "time"; });
	if (driverBridgeInput != driverBridgeInputs.end())
	{
		vector<VuoCable *> driverBridgeInputIncomingCables = (*driverBridgeInput)->getConnectedCables();

		vector<VuoPublishedPort *> compositionPublishedInputs = composition->getBase()->getPublishedInputPorts();
		for (VuoPublishedPort *compositionPublishedInput : compositionPublishedInputs)
		{
			string name = compositionPublishedInput->getClass()->getName();
			auto matchingDriverBridgeInput = std::find_if(driverBridgeInputs.begin(), driverBridgeInputs.end(),
														  [&name] (VuoPublishedPort *p) { return p->getClass()->getName() == name; });
			if (matchingDriverBridgeInput != driverBridgeInputs.end())
				continue;

			VuoType *type = static_cast<VuoCompilerPublishedPort *>(compositionPublishedInput->getCompiler())->getDataVuoType();
			if (! type)
				continue;

			vector<VuoCable *> publishedInputOutgoingCables = compositionPublishedInput->getConnectedCables();

			// Workaround for data types that don't support `Allow Changes`: Connect an event cable from the driver's `time`
			// published output port directly to each internal input port connected to the published port.
			if (canPublishedInputsBeEdited && ! type->getCompiler()->supportsComparison())
			{
				for (VuoCable *driverCable : driverBridgeInputIncomingCables)
				{
					for (VuoCable *compositionCable : publishedInputOutgoingCables)
					{
						// Event cable: driver output port -> composition input port
						VuoCompilerNode *fromCompilerNode = driverCable->getFromNode()->getCompiler();
						VuoCompilerPort *fromCompilerPort = static_cast<VuoCompilerPort *>(driverCable->getFromPort()->getCompiler());
						VuoCompilerNode *toCompilerNode = compositionCable->getToNode()->getCompiler();
						VuoCompilerPort *toCompilerPort = static_cast<VuoCompilerPort *>(compositionCable->getToPort()->getCompiler());
						VuoCompilerCable *cable = new VuoCompilerCable(fromCompilerNode, fromCompilerPort, toCompilerNode, toCompilerPort);
						cable->setAlwaysEventOnly(true);
						composition->getBase()->addCable(cable->getBase());
					}
				}
				continue;
			}

			// Node: Allow Changes or Allow First Value
			string allowNodeClassName = (canPublishedInputsBeEdited ? "vuo.event.allowChanges2" : "vuo.event.allowFirstValue");
			VuoCompilerNodeClass *allowNodeClass = compiler->getNodeClass(allowNodeClassName + "." + type->getModuleKey());
			VuoNode *allowNode = compiler->createNode(allowNodeClass);
			string nodeIdentifierPrefix = allowNode->getCompiler()->getGraphvizIdentifierPrefix() + "_Driver";
			composition->setUniqueGraphvizIdentifierForNode(allowNode, nodeIdentifierPrefix, nodeIdentifierPrefix);
			composition->getBase()->addNode(allowNode);

			VuoCompilerPort *allowInputPort = static_cast<VuoCompilerPort *>(allowNode->getInputPorts().at(VuoNodeClass::unreservedInputPortStartIndex)->getCompiler());
			VuoCompilerPort *allowOutputPort = static_cast<VuoCompilerPort *>(allowNode->getOutputPorts().at(VuoNodeClass::unreservedOutputPortStartIndex)->getCompiler());

			{
				// Data cable: composition published input port -> Allow node
				VuoCompilerPort *fromCompilerPort = static_cast<VuoCompilerPublishedPort *>(compositionPublishedInput->getCompiler());
				VuoCompilerCable *cable = new VuoCompilerCable(nullptr, fromCompilerPort, allowNode->getCompiler(), allowInputPort);
				composition->getBase()->addCable(cable->getBase());
			}

			for (VuoCable *driverCable : driverBridgeInputIncomingCables)
			{
				// Event cable: driver output port -> Allow node
				VuoCompilerNode *fromCompilerNode = driverCable->getFromNode()->getCompiler();
				VuoCompilerPort *fromCompilerPort = static_cast<VuoCompilerPort *>(driverCable->getFromPort()->getCompiler());
				VuoCompilerCable *cable = new VuoCompilerCable(fromCompilerNode, fromCompilerPort, allowNode->getCompiler(), allowInputPort);
				cable->setAlwaysEventOnly(true);
				composition->getBase()->addCable(cable->getBase());
			}

			for (VuoCable *compositionCable : publishedInputOutgoingCables)
			{
				// Event cable: Allow node -> composition input port
				VuoCompilerNode *toCompilerNode = compositionCable->getToNode()->getCompiler();
				VuoCompilerPort *toCompilerPort = static_cast<VuoCompilerPort *>(compositionCable->getToPort()->getCompiler());
				VuoCompilerCable *cable = new VuoCompilerCable(allowNode->getCompiler(), allowOutputPort, toCompilerNode, toCompilerPort);
				cable->setAlwaysEventOnly(true);
				composition->getBase()->addCable(cable->getBase());
			}
		}
	}
}
