/**
 * @file
 * VuoCompilerComposition implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerComposition.hh"
#include "VuoCompilerCable.hh"
#include "VuoFileUtilities.hh"
#include "VuoPort.hh"
#include "VuoStringUtilities.hh"
#include <sstream>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json/json.h>
#pragma clang diagnostic pop


const string VuoCompilerComposition::defaultGraphDeclaration = "digraph G\n";

/**
 * Creates a composition. If a non-null parser is provided, the composition is populated from the parser.
 * Otherwise, the composition is empty.
 */
VuoCompilerComposition::VuoCompilerComposition(VuoComposition *baseComposition, VuoCompilerGraphvizParser *parser)
	: VuoBaseDetail<VuoComposition>("VuoCompilerComposition", baseComposition)
{
	getBase()->setCompiler(this);

	publishedInputNode = NULL;
	publishedOutputNode = NULL;

	if (parser)
	{
		vector<VuoNode *> nodes = parser->getNodes();
		for (vector<VuoNode *>::iterator node = nodes.begin(); node != nodes.end(); ++node)
			getBase()->addNode(*node);

		vector<VuoCable *> cables = parser->getCables();
		for (vector<VuoCable *>::iterator cable = cables.begin(); cable != cables.end(); ++cable)
			getBase()->addCable(*cable);

		vector<VuoCable *> publishedInputCables = parser->getPublishedInputCables();
		for (vector<VuoCable *>::iterator cable = publishedInputCables.begin(); cable != publishedInputCables.end(); ++cable)
			getBase()->addPublishedInputCable(*cable);

		vector<VuoCable *> publishedOutputCables = parser->getPublishedOutputCables();
		for (vector<VuoCable *>::iterator cable = publishedOutputCables.begin(); cable != publishedOutputCables.end(); ++cable)
			getBase()->addPublishedOutputCable(*cable);

		vector<VuoCompilerPublishedPort *> publishedInputPorts = parser->getPublishedInputPorts();
		for (int index = 0; index < publishedInputPorts.size(); ++index)
			getBase()->addPublishedInputPort(publishedInputPorts.at(index)->getBase(), index);

		vector<VuoCompilerPublishedPort *> publishedOutputPorts = parser->getPublishedOutputPorts();
		for (int index = 0; index < publishedOutputPorts.size(); ++index)
			getBase()->addPublishedOutputPort(publishedOutputPorts.at(index)->getBase(), index);

		vector<string> unknownNodeClasses = parser->getUnknownNodeClasses();
		if (! unknownNodeClasses.empty())
		{
			for (vector<string>::iterator i = unknownNodeClasses.begin(), e = unknownNodeClasses.end(); i != e; ++i)
				fprintf(stderr, "Couldn't find implementation for node class %s\n", (*i).c_str());
		}

		publishedInputNode = parser->getPublishedInputNode();
		publishedOutputNode = parser->getPublishedOutputNode();

		getBase()->setDescription(parser->getDescription());
	}
}

/**
 * Creates a composition from the Graphviz-formatted string representation of a composition.
 */
VuoCompilerComposition * VuoCompilerComposition::newCompositionFromGraphvizDeclaration(const string &compositionGraphvizDeclaration, VuoCompiler *compiler)
{
	FILE *file = VuoFileUtilities::stringToCFile(compositionGraphvizDeclaration.c_str());
	VuoCompilerGraphvizParser parser(file, compiler);
	fclose(file);
	return new VuoCompilerComposition(new VuoComposition(), &parser);
}

/**
 * Returns the psuedo-node that contains the published input ports, or @c NULL if this composition has no published input ports.
 */
VuoNode * VuoCompilerComposition::getPublishedInputNode(void)
{
	return publishedInputNode;
}

/**
 * Returns the psuedo-node that contains the published output ports, or @c NULL if this composition has no published output ports.
 */
VuoNode * VuoCompilerComposition::getPublishedOutputNode(void)
{
	return publishedOutputNode;
}

/**
 * Sets the psuedo-node that contains the published input ports.
 */
void VuoCompilerComposition::setPublishedInputNode(VuoNode *node)
{
	this->publishedInputNode = node;
}

/**
 * Sets the psuedo-node that contains the published output ports.
 */
void VuoCompilerComposition::setPublishedOutputNode(VuoNode *node)
{
	this->publishedOutputNode = node;
}

/**
 * Returns the .vuo (Graphviz dot format) representation of this composition.
 */
string VuoCompilerComposition::getGraphvizDeclaration(string header, string footer)
{
	return getGraphvizDeclarationForComponents(getBase()->getNodes(), getBase()->getCables(), header, footer);
}

/**
 * Returns the .vuo (Graphviz dot format) representation of the given nodes and cables in this composition.
 */
string VuoCompilerComposition::getGraphvizDeclarationForComponents(set<VuoNode *> nodeSet, set<VuoCable *> cableSet,
																   string header, string footer, double xPositionOffset, double yPositionOffset)
{
	// Sort nodes.
	vector<VuoNode *> nodes;
	for (set<VuoNode *>::iterator i = nodeSet.begin(); i != nodeSet.end(); ++i)
		nodes.push_back(*i);

	sort(nodes.begin(), nodes.end(), compareGraphvizIdentifiersOfNodes);

	// Sort cables.
	vector<VuoCable *> cables;
	for (set<VuoCable *>::iterator i = cableSet.begin(); i != cableSet.end(); ++i)
		cables.push_back(*i);

	sort(cables.begin(), cables.end(), compareGraphvizIdentifiersOfCables);

	string compositionHeader = (! header.empty()? header : defaultGraphDeclaration);
	string compositionFooter = (! footer.empty()? footer : "\n");

	// Determine which of the composition's published ports are contained within the subcomposition.
	set<VuoPublishedPort *> subcompositionPublishedInputPorts;
	set<VuoPublishedPort *> subcompositionPublishedOutputPorts;
	for (vector<VuoNode *>::iterator node = nodes.begin(); node != nodes.end(); ++node)
	{
		set<pair<VuoPublishedPort *, VuoPort *> > publishedInputPortsOfNode = getBase()->getPublishedInputPortsConnectedToNode((*node));
		for (set<pair<VuoPublishedPort *, VuoPort *> >::iterator i = publishedInputPortsOfNode.begin(); i != publishedInputPortsOfNode.end(); ++i)
			subcompositionPublishedInputPorts.insert(i->first);

		set<pair<VuoPublishedPort *, VuoPort *> > publishedOutputPortsOfNode = getBase()->getPublishedOutputPortsConnectedToNode((*node));
		for (set<pair<VuoPublishedPort *, VuoPort *> >::iterator i = publishedOutputPortsOfNode.begin(); i != publishedOutputPortsOfNode.end(); ++i)
			subcompositionPublishedOutputPorts.insert(i->first);
	}

	ostringstream output;
	string nodeCableSectionDivider = (! (cables.empty()
										 && subcompositionPublishedInputPorts.empty()
										 && subcompositionPublishedOutputPorts.empty())?
										 "\n":"");

	// Print header
	output << compositionHeader;
	output << "{" << endl;

	// Print nodes
	for (vector<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
		output << (*i)->getCompiler()->getGraphvizDeclaration(true, xPositionOffset, yPositionOffset) << endl;

	// Print psuedo-nodes for published ports
	if (! subcompositionPublishedInputPorts.empty())
	{
		output << VuoNodeClass::publishedInputNodeIdentifier << " [type=\"" << VuoNodeClass::publishedInputNodeClassName << "\" label=\"" << VuoNodeClass::publishedInputNodeIdentifier;
		for (set<VuoPublishedPort *>::iterator i = subcompositionPublishedInputPorts.begin(); i != subcompositionPublishedInputPorts.end(); ++i)
			output << "|<" << (*i)->getName() << ">" << (*i)->getName() << "\\r";
		output << "\"";
		for (set<VuoPublishedPort *>::iterator i = subcompositionPublishedInputPorts.begin(); i != subcompositionPublishedInputPorts.end(); ++i)
		{
			VuoPort *connectedPort = *(*i)->getConnectedPorts().begin();
			VuoCompilerInputData *data = static_cast<VuoCompilerInputEventPort *>(connectedPort->getCompiler())->getData();
			if (data)
			{
				string portConstant = data->getInitialValue();
				string escapedPortConstant = VuoStringUtilities::transcodeToGraphvizIdentifier(portConstant);
				output << " _" << (*i)->getName() << "=\"" << escapedPortConstant << "\"";
			}
		}
		output << "];" << endl;
	}
	if (! subcompositionPublishedOutputPorts.empty())
	{
		output << VuoNodeClass::publishedOutputNodeIdentifier <<  " [type=\"" << VuoNodeClass::publishedOutputNodeClassName << "\" label=\"" << VuoNodeClass::publishedOutputNodeIdentifier;
		for (set<VuoPublishedPort *>::iterator i = subcompositionPublishedOutputPorts.begin(); i != subcompositionPublishedOutputPorts.end(); ++i)
			output << "|<" << (*i)->getName() << ">" << (*i)->getName() << "\\l";
		output << "\"];" << endl;
	}

	output << nodeCableSectionDivider;

	// Print cables
	for (vector<VuoCable *>::iterator cable = cables.begin(); cable != cables.end(); ++cable)
		output << (*cable)->getCompiler()->getGraphvizDeclaration() << endl;

	// Print pseudo-cables for published ports
	for (set<VuoPublishedPort *>::iterator publishedPort = subcompositionPublishedInputPorts.begin(); publishedPort != subcompositionPublishedInputPorts.end(); ++publishedPort)
	{
		set<VuoPort *> connectedPorts = (*publishedPort)->getConnectedPorts();
		for (set<VuoPort *>::iterator connectedPort = connectedPorts.begin(); connectedPort != connectedPorts.end(); ++connectedPort)
		{
			string connectedPortName = (*connectedPort)->getClass()->getName();
			for (vector<VuoNode *>::iterator node = nodes.begin(); node != nodes.end(); ++node)
			{
				if ((*node)->getInputPortWithName(connectedPortName) == *connectedPort)
				{
					output	<< VuoNodeClass::publishedInputNodeIdentifier << ":" << (*publishedPort)->getName() << " -> "
							<< (*node)->getCompiler()->getGraphvizIdentifier() << ":" << connectedPortName << ";" << endl;
					break;
				}
			}
		}
	}
	for (set<VuoPublishedPort *>::iterator publishedPort = subcompositionPublishedOutputPorts.begin(); publishedPort != subcompositionPublishedOutputPorts.end(); ++publishedPort)
	{
		set<VuoPort *> connectedPorts = (*publishedPort)->getConnectedPorts();
		for (set<VuoPort *>::iterator connectedPort = connectedPorts.begin(); connectedPort != connectedPorts.end(); ++connectedPort)
		{
			string connectedPortName = (*connectedPort)->getClass()->getName();
			for (vector<VuoNode *>::iterator node = nodes.begin(); node != nodes.end(); ++node)
			{
				if ((*node)->getOutputPortWithName(connectedPortName) == *connectedPort)
				{
					output	<< (*node)->getCompiler()->getGraphvizIdentifier() << ":" << connectedPortName << " -> "
							<< VuoNodeClass::publishedOutputNodeIdentifier << ":" << (*publishedPort)->getName() << ";" << endl;
					break;
				}
			}
		}
	}

	output << "}";
	output << compositionFooter;

	return output.str();
}

/**
 * Returns a string representation of a comparison between the old and the current composition.
 *
 * This needs to be kept in sync with VuoRuntime function isNodeInBothCompositions().
 *
 * The string representation has the [JSON Patch](http://tools.ietf.org/html/draft-ietf-appsawg-json-patch-02) format.
 * The key used for each node is its Graphviz identifier. Unlike the example below (spaced for readability), the
 * returned string contains no whitespace.
 *
 * @eg{
 * [
 *   {"add" : "FireOnStart", "value" : {"nodeClass" : "vuo.event.fireOnStart"}},
 *   {"remove" : "Round"}
 * ]
 * }
 */
string VuoCompilerComposition::diffAgainstOlderComposition(string oldCompositionGraphvizDeclaration, VuoCompiler *compiler)
{
	json_object *diff = json_object_new_array();

	VuoCompilerComposition *oldComposition = newCompositionFromGraphvizDeclaration(oldCompositionGraphvizDeclaration, compiler);
	set<VuoNode *> oldNodes = oldComposition->getBase()->getNodes();
	set<VuoNode *> newNodes = getBase()->getNodes();

	map<string, VuoNode *> oldNodeForIdentifier;
	map<string, VuoNode *> newNodeForIdentifier;
	for (set<VuoNode *>::iterator i = oldNodes.begin(); i != oldNodes.end(); ++i)
		oldNodeForIdentifier[(*i)->getCompiler()->getGraphvizIdentifier()] = *i;
	for (set<VuoNode *>::iterator i = newNodes.begin(); i != newNodes.end(); ++i)
		newNodeForIdentifier[(*i)->getCompiler()->getGraphvizIdentifier()] = *i;

	for (map<string, VuoNode *>::iterator oldNodeIter = oldNodeForIdentifier.begin(); oldNodeIter != oldNodeForIdentifier.end(); ++oldNodeIter)
	{
		map<string, VuoNode *>::iterator newNodeIter = newNodeForIdentifier.find(oldNodeIter->first);
		if (newNodeIter == newNodeForIdentifier.end())
		{
			// { "remove" : "<node identifier>" }
			json_object *remove = json_object_new_object();
			json_object *nodeIdentifier = json_object_new_string(oldNodeIter->first.c_str());
			json_object_object_add(remove, "remove", nodeIdentifier);
			json_object_array_add(diff, remove);
		}
	}
	for (map<string, VuoNode *>::iterator newNodeIter = newNodeForIdentifier.begin(); newNodeIter != newNodeForIdentifier.end(); ++newNodeIter)
	{
		map<string, VuoNode *>::iterator oldNodeIter = oldNodeForIdentifier.find(newNodeIter->first);
		if (oldNodeIter == oldNodeForIdentifier.end())
		{
			// { "add" : "<node identifier>", "value" : { "nodeClass" : "<node class>" } }
			json_object *add = json_object_new_object();
			json_object *nodeIdentifier = json_object_new_string(newNodeIter->first.c_str());
			json_object_object_add(add, "add", nodeIdentifier);
			json_object *value = json_object_new_object();
			json_object *nodeClass = json_object_new_string(newNodeIter->second->getNodeClass()->getClassName().c_str());
			json_object_object_add(value, "nodeClass", nodeClass);
			json_object_object_add(add, "value", value);
			json_object_array_add(diff, add);
		}
	}

	delete oldComposition;

	string diffString = json_object_to_json_string_ext(diff, JSON_C_TO_STRING_PLAIN);
	json_object_put(diff);
	return diffString;
}

/**
 * Returns true if @c lhs precedes @c rhs in lexicographic order of their identifiers in a .vuo file.
 */
bool VuoCompilerComposition::compareGraphvizIdentifiersOfNodes(VuoNode *lhs, VuoNode *rhs)
{
	string lhsIdentifier = (lhs->hasCompiler() ? lhs->getCompiler()->getGraphvizIdentifier() : "");
	string rhsIdentifier = (rhs->hasCompiler() ? rhs->getCompiler()->getGraphvizIdentifier() : "");
	return lhsIdentifier.compare(rhsIdentifier) < 0;
}

/**
 * Returns true if @c lhs precedes @c rhs in lexicographic order of their identifiers in a .vuo file.
 */
bool VuoCompilerComposition::compareGraphvizIdentifiersOfCables(VuoCable *lhs, VuoCable *rhs)
{
	string lhsIdentifier = (lhs->hasCompiler() ? lhs->getCompiler()->getGraphvizDeclaration() : "");
	string rhsIdentifier = (rhs->hasCompiler() ? rhs->getCompiler()->getGraphvizDeclaration() : "");
	return lhsIdentifier.compare(rhsIdentifier) < 0;
}
