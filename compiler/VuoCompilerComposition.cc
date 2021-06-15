/**
 * @file
 * VuoCompilerComposition implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCable.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerComment.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerGraph.hh"
#include "VuoCompilerGraphvizParser.hh"
#include "VuoCompilerIssue.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerSpecializedNodeClass.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoComposition.hh"
#include "VuoGenericType.hh"
#include "VuoNode.hh"
#include "VuoNodeClass.hh"
#include "VuoComment.hh"
#include "VuoPublishedPort.hh"
#include "VuoStringUtilities.hh"
#include <sstream>

const string VuoCompilerComposition::defaultGraphDeclaration = "digraph G\n";
const string VuoCompilerComposition::topLevelCompositionIdentifier = "Top";

/**
 * Creates a composition. If a non-null parser is provided, the composition is populated from the parser.
 * Otherwise, the composition is empty.
 */
VuoCompilerComposition::VuoCompilerComposition(VuoComposition *baseComposition, VuoCompilerGraphvizParser *parser)
	: VuoBaseDetail<VuoComposition>("VuoCompilerComposition", baseComposition)
{
	getBase()->setCompiler(this);

	graph = nullptr;
	graphHash = 0;
	manuallyFirableInputNode = nullptr;
	manuallyFirableInputPort = nullptr;
	module = nullptr;

	if (parser)
	{
		vector<VuoNode *> nodes = parser->getNodes();
		for (vector<VuoNode *>::iterator node = nodes.begin(); node != nodes.end(); ++node)
			getBase()->addNode(*node);

		vector<VuoCable *> cables = parser->getCables();
		for (vector<VuoCable *>::iterator cable = cables.begin(); cable != cables.end(); ++cable)
			getBase()->addCable(*cable);

		vector<VuoPublishedPort *> publishedInputPorts = parser->getPublishedInputPorts();
		for (int index = 0; index < publishedInputPorts.size(); ++index)
			getBase()->addPublishedInputPort(publishedInputPorts.at(index), index);

		vector<VuoPublishedPort *> publishedOutputPorts = parser->getPublishedOutputPorts();
		for (int index = 0; index < publishedOutputPorts.size(); ++index)
			getBase()->addPublishedOutputPort(publishedOutputPorts.at(index), index);

		vector<VuoComment *> comments = parser->getComments();
		for (vector<VuoComment *>::iterator comment = comments.begin(); comment != comments.end(); ++comment)
			getBase()->addComment(*comment);

		manuallyFirableInputNode = parser->getManuallyFirableInputNode();
		manuallyFirableInputPort = parser->getManuallyFirableInputPort();

		getBase()->setMetadata(parser->getMetadata(), true);

		updateGenericPortTypes();

		for (vector<VuoNode *>::iterator node = nodes.begin(); node != nodes.end(); ++node)
			if ((*node)->hasCompiler())
				nodeGraphvizIdentifierUsed[ (*node)->getCompiler()->getGraphvizIdentifier() ] = *node;
	}
}

/**
 * Destructor.
 */
VuoCompilerComposition::~VuoCompilerComposition(void)
{
	delete graph;
	VuoCompiler::destroyLlvmModule(module);
}

/**
 * Creates a composition from the Graphviz-formatted string representation of a composition.
 *
 * @throw VuoCompilerException Couldn't parse the composition.
 */
VuoCompilerComposition * VuoCompilerComposition::newCompositionFromGraphvizDeclaration(const string &compositionGraphvizDeclaration, VuoCompiler *compiler)
{
	VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionString(compositionGraphvizDeclaration, compiler);
	VuoCompilerComposition *composition = new VuoCompilerComposition(new VuoComposition(), parser);
	delete parser;
	return composition;
}

/**
 * Outputs the modifications to cables that would be made by @ref VuoCompilerComposition::replaceNode
 * without actually modifying anything.
 */
void VuoCompilerComposition::getChangesToReplaceNode(VuoNode *oldNode, VuoNode *newNode,
													 map<VuoCable *, VuoPort *> &cablesToTransferFromPort,
													 map<VuoCable *, VuoPort *> &cablesToTransferToPort,
													 set<VuoCable *> &cablesToRemove) const
{
	set<VuoCable *> cables = getBase()->getCables();
	for (set<VuoCable *>::iterator i = cables.begin(); i != cables.end(); ++i)
	{
		VuoCable *cable = *i;

		bool foundMismatch = false;
		if (cable->getFromNode() == oldNode)
		{
			VuoPort *oldPort = cable->getFromPort();
			VuoPort *newPort = newNode->getOutputPortWithName( oldPort->getClass()->getName() );

			if (portsMatch(oldPort, newPort))
				cablesToTransferFromPort[cable] = newPort;
			else
				foundMismatch = true;
		}
		if (cable->getToNode() == oldNode)
		{
			VuoPort *oldPort = cable->getToPort();
			VuoPort *newPort = newNode->getInputPortWithName( oldPort->getClass()->getName() );

			if (portsMatch(oldPort, newPort))
				cablesToTransferToPort[cable] = newPort;
			else
				foundMismatch = true;
		}

		if (foundMismatch)
		{
			cablesToRemove.insert(cable);

			map<VuoCable *, VuoPort *>::iterator fromIter = cablesToTransferFromPort.find(cable);
			if (fromIter != cablesToTransferFromPort.end())
				cablesToTransferFromPort.erase(fromIter);

			map<VuoCable *, VuoPort *>::iterator toIter = cablesToTransferToPort.find(cable);
			if (toIter != cablesToTransferToPort.end())
				cablesToTransferToPort.erase(toIter);
		}
	}
}

/**
 * Replaces @a oldNode with @a newNode in the composition, transferring all cable and published port
 * connections from @a oldNode to @a newNode where port names and data types correspond, and severing the rest.
 *
 * If @a oldNode or @a newNode lacks a compiler detail (its node class is not loaded), then cables are
 * transferred where port names correspond.
 */
void VuoCompilerComposition::replaceNode(VuoNode *oldNode, VuoNode *newNode)
{
	map<VuoCable *, VuoPort *> cablesToTransferFromPort;
	map<VuoCable *, VuoPort *> cablesToTransferToPort;
	set<VuoCable *> cablesToRemove;

	getChangesToReplaceNode(oldNode, newNode, cablesToTransferFromPort, cablesToTransferToPort, cablesToRemove);

	getBase()->removeNode(oldNode);
	getBase()->addNode(newNode);

	for (map<VuoCable *, VuoPort *>::iterator i = cablesToTransferFromPort.begin(); i != cablesToTransferFromPort.end(); ++i)
		i->first->setFrom(newNode, i->second);
	for (map<VuoCable *, VuoPort *>::iterator i = cablesToTransferToPort.begin(); i != cablesToTransferToPort.end(); ++i)
		i->first->setTo(newNode, i->second);
	for (set<VuoCable *>::iterator i = cablesToRemove.begin(); i != cablesToRemove.end(); ++i)
		getBase()->removeCable(*i);
}

/**
 * Returns the graph for this composition, using the most recently generated graph if it still applies.
 */
VuoCompilerGraph * VuoCompilerComposition::getCachedGraph(VuoCompiler *compiler)
{
	long currentHash = VuoCompilerGraph::getHash(this);
	bool compositionChanged = (currentHash != graphHash);

	bool shouldAddPublishedNodeImplementations = false;
	if (! compositionChanged && compiler)
	{
		VuoCompilerNode *publishedInputNode = graph->getPublishedInputNode();
		if (publishedInputNode && ! publishedInputNode->getBase()->getNodeClass()->getCompiler()->getEventFunction())
			shouldAddPublishedNodeImplementations = true;
	}

	if (compositionChanged || shouldAddPublishedNodeImplementations)
	{
		delete graph;
		graph = new VuoCompilerGraph(this, compiler);
		graphHash = currentHash;
	}

	return graph;
}

/**
 * Indicates that the most recently generated graph for this composition no longer applies.
 */
void VuoCompilerComposition::invalidateCachedGraph(void)
{
	delete graph;
	graph = nullptr;
	graphHash = 0;
}

/**
 * Checks that the composition is valid (able to be compiled).
 *
 * @param issues Any issues found are appended to this.
 * @throw VuoCompilerException The composition is invalid.
 */
void VuoCompilerComposition::check(VuoCompilerIssues *issues)
{
	checkForMissingNodeClasses(issues);
	checkForMissingTypes(issues);
	checkForEventFlowIssues(issues);
}

/**
 * Checks that all of the nodes in the composition have a node class known to the compiler.
 *
 * @param issues Any issues found are appended to this.
 * @throw VuoCompilerException One or more nodes have an unknown node class.
 */
void VuoCompilerComposition::checkForMissingNodeClasses(VuoCompilerIssues *issues)
{
	set<VuoNode *> missingNodes;
	set<VuoNode *> nodes = getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		if (! node->getNodeClass()->hasCompiler())
			missingNodes.insert(node);
	}

	if (! missingNodes.empty())
	{
		// Assumes the details are always displayed in plain text. Doesn't use VuoCompilerIssue's formatting.
		vector<string> uniqueNodeDetails;
		bool missingProNode = false;
		bool missingOldNode = false;
		for (set<VuoNode *>::iterator i = missingNodes.begin(); i != missingNodes.end(); ++i)
		{
			VuoNode *node = *i;

			string nodeDetail = node->getTitle() + " (" + (*i)->getNodeClass()->getClassName() + ")";

#if VUO_PRO
			if (node->getNodeClass()->isPro())
			{
				nodeDetail += " [Vuo Pro]";
				missingProNode = true;
			}
#endif

			if (node->getNodeClass()->getDescription().find("This node was updated or removed in Vuo 0.9 or earlier.") != string::npos)
			{
				nodeDetail += " [Vuo 0.9 or earlier]";
				missingOldNode = true;
			}

			if (find(uniqueNodeDetails.begin(), uniqueNodeDetails.end(), nodeDetail) == uniqueNodeDetails.end())
				uniqueNodeDetails.push_back(nodeDetail);
		}
		sort(uniqueNodeDetails.begin(), uniqueNodeDetails.end());
		string details = "\n\n" + VuoStringUtilities::join(uniqueNodeDetails, "\n");

		string hint;
		string linkUrl;
		string linkText;

		if (missingProNode)
		{
			hint += "<p>Some of the non-installed nodes are Pro nodes. "
					"%link to enable Pro nodes.</p>";
			linkUrl = "https://vuo.org/buy";
			linkText = "Upgrade to Vuo Pro";
		}

		if (missingOldNode)
		{
			hint += "<p>Some of the non-installed nodes were updated or removed in Vuo 0.9 or earlier. "
					"To work with this composition in Vuo 1.0 or later, first make a "
					"backup copy of the composition, then open the composition in Vuo 0.9 and save it. "
					"This will automatically upgrade the composition so you can use it in Vuo 1.0 or later.</p>";
		}

		// If changing this text, also change VuoEditorWindow::hideBuildActivityIndicator() and VuoEditorWindow::displayExportErrorBox().
		string summary = "Nodes not installed";

		VuoCompilerIssue issue(VuoCompilerIssue::Error, "opening composition", "", summary, details);
		issue.setNodes(missingNodes);
		issue.setHint(hint);
		issue.setLink(linkUrl, linkText);
		if (issues)
		{
			issues->append(issue);
			throw VuoCompilerException(issues, false);
		}
		else
			throw VuoCompilerException(issue);
	}
}

/**
 * Checks that all of the nodes in the composition have port types known to the compiler.
 *
 * @param issues Any issues found are appended to this.
 * @throw VuoCompilerException One or more nodes have an unknown port type.
 */
void VuoCompilerComposition::checkForMissingTypes(VuoCompilerIssues *issues)
{
	map<string, set<string> > missingTypes;
	for (VuoNode *node : getBase()->getNodes())
	{
		if (! node->getNodeClass()->hasCompiler())
			continue;

		vector<VuoPort *> ports;
		vector<VuoPort *> inputPorts = node->getInputPorts();
		ports.insert(ports.end(), inputPorts.begin(), inputPorts.end());
		vector<VuoPort *> outputPorts = node->getOutputPorts();
		ports.insert(ports.end(), outputPorts.begin(), outputPorts.end());

		for (VuoPort *port : ports)
		{
			VuoType *type = static_cast<VuoCompilerPort *>(port->getCompiler())->getDataVuoType();
			if (type && ! type->hasCompiler() && ! dynamic_cast<VuoGenericType *>(type))
				missingTypes[type->getModuleKey()].insert(node->getNodeClass()->getClassName());
		}
	}

	if (! missingTypes.empty())
	{
		auto exceptionIssues = new VuoCompilerIssues;
		for (auto i : missingTypes)
		{
			vector<string> nodeClassNames(i.second.begin(), i.second.end());
			std::sort(nodeClassNames.begin(), nodeClassNames.end());

			string summary = "Data type not installed";
			string details = i.first + " — used by " + VuoStringUtilities::join(nodeClassNames, ", ");
			string hint = "Check with the developer of the nodes that use this data type.";

			VuoCompilerIssue issue(VuoCompilerIssue::Error, "opening composition", "", summary, details);
			issue.setHint(hint);
			exceptionIssues->append(issue);
			if (issues)
				issues->append(issue);
		}

		throw VuoCompilerException(exceptionIssues, true);
	}
}

/**
 * Checks that the event flow in the composition is valid.
 *
 * @param issues Any issues found are appended to this.
 * @throw VuoCompilerException There is an infinite or deadlocked feedback loop.
 */
void VuoCompilerComposition::checkForEventFlowIssues(VuoCompilerIssues *issues)
{
	checkForEventFlowIssues(set<VuoCompilerCable *>(), issues);
}

/**
 * Checks that the event flow in the composition is valid.
 *
 * @param potentialCables Cables that are not yet in the composition but should be included in the check.
 * @param issues Any issues found are appended to this.
 * @throw VuoCompilerException There is an infinite or deadlocked feedback loop.
 */
void VuoCompilerComposition::checkForEventFlowIssues(set<VuoCompilerCable *> potentialCables, VuoCompilerIssues *issues)
{
	VuoCompilerGraph *graph;
	if (! potentialCables.empty())
		graph = new VuoCompilerGraph(this, nullptr, potentialCables);
	else
		graph = getCachedGraph();

	graph->checkForInfiniteFeedback(issues);
	graph->checkForDeadlockedFeedback(issues);

	if (! potentialCables.empty())
		delete graph;
}

/**
 * Puts the generic ports in the composition into sets, where all ports in a set have the same generic type.
 */
set< set<VuoCompilerPort *> > VuoCompilerComposition::groupGenericPortsByType(void)
{
	set< set<VuoCompilerPort *> > setsOfConnectedGenericPorts;

	// Put the generic ports into sets by node, with additional sets for published input and output ports.
	set< pair< vector<VuoPort *>, VuoNode *> > portsGroupedByNode;
	set<VuoNode *> nodes = getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		if (! node->hasCompiler())
			continue;

		vector<VuoPort *> inputPorts = node->getInputPorts();
		vector<VuoPort *> outputPorts = node->getOutputPorts();
		vector<VuoPort *> ports;
		ports.insert(ports.end(), inputPorts.begin(), inputPorts.end());
		ports.insert(ports.end(), outputPorts.begin(), outputPorts.end());

		portsGroupedByNode.insert( make_pair(ports, node) );
	}
	vector<VuoPublishedPort *> publishedInputPorts = getBase()->getPublishedInputPorts();
	vector<VuoPublishedPort *> publishedOutputPorts = getBase()->getPublishedOutputPorts();
	vector<VuoPort *> publishedInputBasePorts( publishedInputPorts.begin(), publishedInputPorts.end() );
	vector<VuoPort *> publishedOutputBasePorts( publishedOutputPorts.begin(), publishedOutputPorts.end() );
	portsGroupedByNode.insert( make_pair(publishedInputBasePorts, static_cast<VuoNode *>(NULL)) );
	portsGroupedByNode.insert( make_pair(publishedOutputBasePorts, static_cast<VuoNode *>(NULL)) );

	// Within each set of generic ports by node (or published ports), group the generic ports into sets by generic type.
	for (set< pair< vector<VuoPort *>, VuoNode* > >::iterator i = portsGroupedByNode.begin(); i != portsGroupedByNode.end(); ++i)
	{
		vector<VuoPort *> ports = i->first;

		map<string, set<VuoCompilerPort *> > genericPortsForType;
		for (vector<VuoPort *>::iterator j = ports.begin(); j != ports.end(); ++j)
		{
			VuoCompilerPort *port = static_cast<VuoCompilerPort *>((*j)->getCompiler());
			VuoGenericType *genericType = dynamic_cast<VuoGenericType *>(port->getDataVuoType());

			if (genericType)
			{
				string innermostGenericTypeName = VuoType::extractInnermostTypeName(genericType->getModuleKey());
				genericPortsForType[innermostGenericTypeName].insert(port);
			}
		}

		for (map<string, set<VuoCompilerPort *> >::iterator j = genericPortsForType.begin(); j != genericPortsForType.end(); ++j)
		{
			set<VuoCompilerPort *> genericPorts = j->second;
			setsOfConnectedGenericPorts.insert(genericPorts);
		}
	}

	// Merge sets of generic ports from different nodes that are connected through a data-carrying cable.
	set<VuoCable *> cables = getBase()->getCables();
	for (set<VuoCable *>::iterator i = cables.begin(); i != cables.end(); ++i)
	{
		VuoCable *cable = *i;

		if (! (cable->getFromPort() && cable->getFromPort()->hasCompiler() &&
			   cable->getToPort() && cable->getToPort()->hasCompiler() &&
			   cable->getCompiler()->carriesData()))
			continue;

		VuoCompilerPort *fromPort = static_cast<VuoCompilerPort *>(cable->getFromPort()->getCompiler());
		VuoCompilerPort *toPort = static_cast<VuoCompilerPort *>(cable->getToPort()->getCompiler());
		set< set<VuoCompilerPort *> >::iterator fromSetIter = setsOfConnectedGenericPorts.end();  // the set containing fromPort
		set< set<VuoCompilerPort *> >::iterator toSetIter = setsOfConnectedGenericPorts.end();  // the set containing toPort
		for (set< set<VuoCompilerPort *> >::iterator j = setsOfConnectedGenericPorts.begin(); j != setsOfConnectedGenericPorts.end(); ++j)
		{
			if (j->find(fromPort) != j->end())
				fromSetIter = j;
			if (j->find(toPort) != j->end())
				toSetIter = j;
		}
		if (fromSetIter != setsOfConnectedGenericPorts.end() && toSetIter != setsOfConnectedGenericPorts.end() &&
				fromSetIter != toSetIter)
		{
			set<VuoCompilerPort *> mergedSet;
			mergedSet.insert(fromSetIter->begin(), fromSetIter->end());
			mergedSet.insert(toSetIter->begin(), toSetIter->end());
			setsOfConnectedGenericPorts.insert(mergedSet);
			setsOfConnectedGenericPorts.erase(fromSetIter);
			setsOfConnectedGenericPorts.erase(toSetIter);
		}
	}

	return setsOfConnectedGenericPorts;
}

/**
 * Gives each group/network of connected generic ports a unique generic type.
 *
 * This does not update the backing types for the generic types. Before compiling the composition,
 * VuoCompiler::reifyGenericPortTypes() needs to be called to update them.
 */
void VuoCompilerComposition::updateGenericPortTypes(void)
{
	set< set<VuoCompilerPort *> > setsOfConnectedGenericPorts = groupGenericPortsByType();

	// Give each set of connected generic ports a unique generic type.
	set<string> usedTypeNames;
	for (set< set<VuoCompilerPort *> >::iterator i = setsOfConnectedGenericPorts.begin(); i != setsOfConnectedGenericPorts.end(); ++i)
	{
		set<VuoCompilerPort *> connectedGenericPorts = *i;

		// Find the smallest-numbered generic type within the set that isn't already used by another set.
		// If no such type exists, create a fresh type.

		vector<string> sortedTypeNames;
		for (set<VuoCompilerPort *>::iterator j = connectedGenericPorts.begin(); j != connectedGenericPorts.end(); ++j)
		{
			VuoCompilerPort *port = *j;
			VuoGenericType *genericTypeFromPort = static_cast<VuoGenericType *>(port->getDataVuoType());
			VuoCompilerPortClass *portClass = static_cast<VuoCompilerPortClass *>(port->getBase()->getClass()->getCompiler());
			VuoGenericType *genericTypeFromPortClass = static_cast<VuoGenericType *>(portClass->getDataVuoType());
			if (genericTypeFromPort != genericTypeFromPortClass)
			{
				string typeName = VuoGenericType::extractInnermostTypeName( genericTypeFromPort->getModuleKey() );
				sortedTypeNames.push_back(typeName);
			}
		}
		VuoGenericType::sortGenericTypeNames(sortedTypeNames);

		string commonTypeName;
		for (vector<string>::iterator j = sortedTypeNames.begin(); j != sortedTypeNames.end(); ++j)
		{
			string portType = *j;
			if (usedTypeNames.find(portType) == usedTypeNames.end())
			{
				commonTypeName = portType;
				break;
			}
		}

		if (commonTypeName.empty())
			commonTypeName = createFreshGenericTypeName();

		usedTypeNames.insert(commonTypeName);

		// Form the set of compatible types for each composition-level generic type by finding the intersection of
		// the sets of compatible types for each node-class-level generic type.
		vector<string> compatibleTypeNames;
		for (set<VuoCompilerPort *>::iterator j = connectedGenericPorts.begin(); j != connectedGenericPorts.end(); ++j)
		{
			VuoCompilerPort *port = *j;
			VuoCompilerPortClass *portClass = static_cast<VuoCompilerPortClass *>(port->getBase()->getClass()->getCompiler());
			VuoGenericType *genericTypeFromPortClass = static_cast<VuoGenericType *>(portClass->getDataVuoType());
			VuoGenericType::Compatibility compatibility;
			vector<string> compatibleTypeNamesForPort = genericTypeFromPortClass->getCompatibleSpecializedTypes(compatibility);
			vector<string> innermostCompatibleTypeNamesForPort;
			for (vector<string>::iterator k = compatibleTypeNamesForPort.begin(); k != compatibleTypeNamesForPort.end(); ++k)
				innermostCompatibleTypeNamesForPort.push_back( VuoType::extractInnermostTypeName(*k) );

			if (! innermostCompatibleTypeNamesForPort.empty())
			{
				if (compatibleTypeNames.empty())
					compatibleTypeNames = innermostCompatibleTypeNamesForPort;
				else
				{
					for (int k = compatibleTypeNames.size() - 1; k >= 0; --k)
						if (find(innermostCompatibleTypeNamesForPort.begin(), innermostCompatibleTypeNamesForPort.end(), compatibleTypeNames[k]) ==
								innermostCompatibleTypeNamesForPort.end())
							compatibleTypeNames.erase(compatibleTypeNames.begin() + k);
				}
			}
		}

		// Apply the composition-level generic type name to each port.
		for (set<VuoCompilerPort *>::iterator j = connectedGenericPorts.begin(); j != connectedGenericPorts.end(); ++j)
		{
			VuoCompilerPort *port = *j;
			VuoCompilerPortClass *portClass = static_cast<VuoCompilerPortClass *>(port->getBase()->getClass()->getCompiler());
			VuoGenericType *genericTypeFromPortClass = static_cast<VuoGenericType *>(portClass->getDataVuoType());

			string typeNameForPort = VuoGenericType::replaceInnermostGenericTypeName(genericTypeFromPortClass->getModuleKey(), commonTypeName);
			vector<string> compatibleTypeNamesForPort;
			string prefix = (VuoType::isListTypeName(typeNameForPort) ? VuoType::listTypeNamePrefix : "");
			for (vector<string>::iterator k = compatibleTypeNames.begin(); k != compatibleTypeNames.end(); ++k)
				compatibleTypeNamesForPort.push_back(prefix + *k);

			VuoGenericType *commonTypeForPort = new VuoGenericType(typeNameForPort, compatibleTypeNamesForPort);
			port->setDataVuoType(commonTypeForPort);
		}
	}

	// Update the list of type suffixes that can't currently be used when creating fresh types.
	for (map<unsigned int, bool>::iterator i = genericTypeSuffixUsed.begin(); i != genericTypeSuffixUsed.end(); ++i)
	{
		unsigned int suffix = i->first;
		bool used = (usedTypeNames.find( VuoGenericType::createGenericTypeName(suffix) ) != usedTypeNames.end());
		genericTypeSuffixUsed[suffix] = used;
	}
}

/**
 * Returns a generic type name that is currently not used within this composition.
 */
string VuoCompilerComposition::createFreshGenericTypeName(void)
{
	for (unsigned int i = 1; ; ++i)
	{
		if (! genericTypeSuffixUsed[i])
		{
			genericTypeSuffixUsed[i] = true;
			return VuoGenericType::createGenericTypeName(i);
		}
	}
}

/**
 * Returns the set of ports that have the same innermost generic type as @a entryPort.
 *
 * Assumes that updateGenericPortTypes() has been called since any changes affecting the groups/networks
 * of connected generic ports have been made to the composition.
 *
 * @param entryNode The node containing @a entryPort.
 * @param entryPort The port whose type is to be matched in the returned ports.
 * @param useOriginalType If true, considers a port generic if it's currently generic or if it's specialized
 *     from a generic. If false, only looks at ports that are currently generic.
 */
set<VuoPort *> VuoCompilerComposition::getCorrelatedGenericPorts(VuoNode *entryNode, VuoPort *entryPort, bool useOriginalType)
{
	/// @todo https://b33p.net/kosada/node/7655 Correctly handle published ports.

	auto getGenericTypeName = [&useOriginalType] (VuoNode *node, VuoPort *port)
	{
		VuoGenericType *genericType = nullptr;

		if (useOriginalType)
		{
			if (node && node->hasCompiler())
			{
				VuoCompilerNodeClass *nodeClass = node->getNodeClass()->getCompiler();
				VuoCompilerSpecializedNodeClass *specializedNodeClass = dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass);
				if (specializedNodeClass)
					genericType = dynamic_cast<VuoGenericType *>(specializedNodeClass->getOriginalPortType(port->getClass()));
			}
		}
		else
		{
			VuoCompilerPort *compilerPort = static_cast<VuoCompilerPort *>(port->getCompiler());
			genericType = dynamic_cast<VuoGenericType *>(compilerPort->getDataVuoType());
		}

		return genericType ? VuoType::extractInnermostTypeName(genericType->getModuleKey()) : "";
	};

	set<VuoPort *> correlatedPorts;
	set<VuoNode *> nodesEnqueuedOrVisited;
	list< pair<VuoNode *, string> > nodesToVisit;

	string entryGenericType = getGenericTypeName(entryNode, entryPort);
	if (! entryGenericType.empty())
	{
		nodesToVisit.push_back( make_pair(entryNode, entryGenericType) );
		nodesEnqueuedOrVisited.insert(entryNode);
	}

	while (! nodesToVisit.empty())
	{
		VuoNode *currNode = nodesToVisit.front().first;
		string currGenericType = nodesToVisit.front().second;
		nodesToVisit.pop_front();

		// Find all ports on the node that share (or would share if unspecialized) the same data type.

		list<VuoPort *> correlatedPortsOnNode;
		for (VuoPort *currPort : currNode->getInputPorts())
			if (getGenericTypeName(currNode, currPort) == currGenericType)
				correlatedPortsOnNode.push_back(currPort);
		for (VuoPort *currPort : currNode->getOutputPorts())
			if (getGenericTypeName(currNode, currPort) == currGenericType)
				correlatedPortsOnNode.push_back(currPort);

		correlatedPorts.insert(correlatedPortsOnNode.begin(), correlatedPortsOnNode.end());

		// Follow all data-carrying cables from those ports to other nodes.

		for (VuoPort *port : correlatedPortsOnNode)
		{
			for (VuoCable *cable : port->getConnectedCables())
			{
				if (! (cable->hasCompiler() && cable->getCompiler()->carriesData()))
					continue;

				VuoNode *otherNode = nullptr;
				VuoPort *otherPort = nullptr;
				if (currNode != cable->getFromNode())
				{
					otherNode = cable->getFromNode();
					otherPort = cable->getFromPort();
				}
				else if (currNode != cable->getToNode())
				{
					otherNode = cable->getToNode();
					otherPort = cable->getToPort();
				}

				if (! otherPort)
					continue;

				if (nodesEnqueuedOrVisited.find(otherNode) == nodesEnqueuedOrVisited.end())
				{
					string otherGenericType = getGenericTypeName(otherNode, otherPort);
					if (! otherGenericType.empty())
					{
						nodesToVisit.push_back( make_pair(otherNode, otherGenericType) );
						nodesEnqueuedOrVisited.insert(otherNode);
					}
				}
			}
		}
	}

	return correlatedPorts;
}

/**
 * Returns the trigger port that is nearest upstream to @a node, or null if no trigger is upstream.
 */
VuoPort * VuoCompilerComposition::findNearestUpstreamTriggerPort(VuoNode *node)
{
	VuoCompilerGraph *graph = getCachedGraph();
	VuoCompilerTriggerPort *trigger = graph->findNearestUpstreamTrigger(node->getCompiler());
	return trigger ? trigger->getBase() : NULL;
}

/**
 * Takes ownership of the LLVM module containing the compiled composition.
 */
void VuoCompilerComposition::setModule(Module *module)
{
	this->module = module;
}

/**
 * Relinquishes ownership of the LLVM module previously passed to @ref setModule.
 */
Module * VuoCompilerComposition::takeModule(void)
{
	Module *takenModule = module;
	module = NULL;
	return takenModule;
}

/**
 * If the given node has the same Graphviz identifier as another node currently in the composition,
 * a node that was in the composition when it was originally parsed from Graphviz, or a node that was
 * previously passed through this function, changes the given node's Graphviz identifier to one that
 * has never been used by this composition.
 */
void VuoCompilerComposition::setUniqueGraphvizIdentifierForNode(VuoNode *node, const string &preferredIdentifier, const string &identifierPrefix)
{
	if (! node->hasCompiler())
		return;

	for (VuoNode *currNode : getBase()->getNodes())
		if (currNode != node && currNode->hasCompiler())
			nodeGraphvizIdentifierUsed[ currNode->getCompiler()->getGraphvizIdentifier() ] = currNode;

	auto isIdentifierAvailable = [this, node] (const string &identifier)
	{
		auto it = nodeGraphvizIdentifierUsed.find(identifier);
		if (it == nodeGraphvizIdentifierUsed.end())
			return true;

		return it->second == node;
	};

	string nonEmptyPreferredIdentifier = (! preferredIdentifier.empty() ? preferredIdentifier : node->getCompiler()->getGraphvizIdentifier());
	string nonEmptyIdentifierPrefix = (! identifierPrefix.empty() ? identifierPrefix : node->getCompiler()->getGraphvizIdentifierPrefix());

	string uniqueIdentifier = VuoStringUtilities::formUniqueIdentifier(isIdentifierAvailable, nonEmptyPreferredIdentifier, nonEmptyIdentifierPrefix);

	nodeGraphvizIdentifierUsed[uniqueIdentifier] = node;
	node->getCompiler()->setGraphvizIdentifier(uniqueIdentifier);
}

/**
 * Clears the map containing the records of previously used Graphviz node identifiers.
 */
void VuoCompilerComposition::clearGraphvizNodeIdentifierHistory()
{
	nodeGraphvizIdentifierUsed.clear();
}

/**
 * If the given comment has the same Graphviz identifier as another comment currently in the composition,
 * a comment that was in the composition when it was originally parsed from Graphviz, or a comment that was
 * previously passed through this function, changes the given comment's Graphviz identifier to one that
 * has never been used by this composition.
 */
void VuoCompilerComposition::setUniqueGraphvizIdentifierForComment(VuoComment *comment)
{
	if (! comment->hasCompiler())
		return;

	for (VuoComment *currComment : getBase()->getComments())
		if (currComment != comment && currComment->hasCompiler())
			commentGraphvizIdentifierUsed[ currComment->getCompiler()->getGraphvizIdentifier() ] = currComment;

	auto isIdentifierAvailable = [this, comment] (const string &identifier)
	{
		auto it = commentGraphvizIdentifierUsed.find(identifier);
		if (it == commentGraphvizIdentifierUsed.end())
			return true;

		return it->second == comment;
	};

	string uniqueIdentifier = VuoStringUtilities::formUniqueIdentifier(isIdentifierAvailable,
																	   comment->getCompiler()->getGraphvizIdentifier(),
																	   comment->getCompiler()->getGraphvizIdentifierPrefix());
	commentGraphvizIdentifierUsed[uniqueIdentifier] = comment;
	comment->getCompiler()->setGraphvizIdentifier(uniqueIdentifier);
}

/**
 * Clears the map containing the records of previously used Graphviz comment identifiers.
 */
void VuoCompilerComposition::clearGraphvizCommentIdentifierHistory()
{
	commentGraphvizIdentifierUsed.clear();
}

/**
 * Selects the input port into which a hidden trigger port can fire events on demand.
 *
 * The trigger port and the cable from it to @a portFiredInto are not included in the
 * lists of nodes or cables stored in the VuoComposition. The trigger port can be accessed
 * with @ref VuoCompilerGraph::getManuallyFirableTrigger().
 */
void VuoCompilerComposition::setManuallyFirableInputPort(VuoNode *nodeContainingPort, VuoPort *portFiredInto)
{
	this->manuallyFirableInputNode = nodeContainingPort;
	this->manuallyFirableInputPort = portFiredInto;
}

/**
 * Returns the node most recently set by @ref VuoCompilerComposition::setManuallyFirableInputPort().
 */
VuoNode * VuoCompilerComposition::getManuallyFirableInputNode(void)
{
	return manuallyFirableInputNode;
}

/**
 * Returns the input port most recently set by @ref VuoCompilerComposition::setManuallyFirableInputPort().
 */
VuoPort * VuoCompilerComposition::getManuallyFirableInputPort(void)
{
	return manuallyFirableInputPort;
}

/**
 * Returns the .vuo (Graphviz dot format) representation of this composition.
 */
string VuoCompilerComposition::getGraphvizDeclaration(VuoProtocol *activeProtocol, string header, string footer)
{
	return getGraphvizDeclarationForComponents(getBase()->getNodes(),
											   getBase()->getCables(),
											   getBase()->getComments(),
											   getBase()->getProtocolAwarePublishedPortOrder(activeProtocol, true),
											   getBase()->getProtocolAwarePublishedPortOrder(activeProtocol, false),
											   header,
											   footer);
}

/**
 * Returns the .vuo (Graphviz dot format) representation of the given nodes and cables in this composition.
 */
string VuoCompilerComposition::getGraphvizDeclarationForComponents(set<VuoNode *> nodeSet,
																   set<VuoCable *> cableSet,
																   set<VuoComment *> commentSet,
																   vector<VuoPublishedPort *> publishedInputPorts,
																   vector<VuoPublishedPort *> publishedOutputPorts,
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

	// Sort comments.
	vector<VuoComment *> comments;
	for (set<VuoComment *>::iterator i = commentSet.begin(); i != commentSet.end(); ++i)
		comments.push_back(*i);

	sort(comments.begin(), comments.end(), compareGraphvizIdentifiersOfComments);

	string compositionHeader = (! header.empty()? header : defaultGraphDeclaration);
	string compositionFooter = (! footer.empty()? footer : "\n");

	ostringstream output;
	string nodeCommentSectionDivider = "\n";
	if ((nodes.empty() && publishedInputPorts.empty() && publishedOutputPorts.empty()) ||
			(comments.empty() && cables.empty()))
		nodeCommentSectionDivider = "";

	string commentCableSectionDivider = "\n";
	if (comments.empty() || cables.empty())
		commentCableSectionDivider = "";

	// Print header
	output << compositionHeader;
	output << "{" << endl;

	// Print nodes
	for (vector<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		string nodeDeclaration = ((*i)->hasCompiler() ?
									  (*i)->getCompiler()->getGraphvizDeclaration(true, xPositionOffset, yPositionOffset,
																				  manuallyFirableInputNode == *i ? manuallyFirableInputPort : nullptr) :
									  (*i)->getRawGraphvizDeclaration());
		output << nodeDeclaration << endl;
	}

	// Print pseudo-nodes for published ports
	if (! publishedInputPorts.empty())
	{
		output << VuoNodeClass::publishedInputNodeIdentifier << " [type=\"" << VuoNodeClass::publishedInputNodeClassName << "\" label=\"" << VuoNodeClass::publishedInputNodeIdentifier;
		for (vector<VuoPublishedPort *>::iterator i = publishedInputPorts.begin(); i != publishedInputPorts.end(); ++i)
			output << "|<" << (*i)->getClass()->getName() << ">" << (*i)->getClass()->getName() << "\\r";
		output << "\"";
		for (vector<VuoPublishedPort *>::iterator i = publishedInputPorts.begin(); i != publishedInputPorts.end(); ++i)
			output << static_cast<VuoCompilerPublishedPort *>((*i)->getCompiler())->getGraphvizAttributes();
		output << "];" << endl;
	}
	if (! publishedOutputPorts.empty())
	{
		output << VuoNodeClass::publishedOutputNodeIdentifier <<  " [type=\"" << VuoNodeClass::publishedOutputNodeClassName << "\" label=\"" << VuoNodeClass::publishedOutputNodeIdentifier;
		for (vector<VuoPublishedPort *>::iterator i = publishedOutputPorts.begin(); i != publishedOutputPorts.end(); ++i)
			output << "|<" << (*i)->getClass()->getName() << ">" << (*i)->getClass()->getName() << "\\l";
		output << "\"";

		for (vector<VuoPublishedPort *>::iterator i = publishedOutputPorts.begin(); i != publishedOutputPorts.end(); ++i)
			output << static_cast<VuoCompilerPublishedPort *>((*i)->getCompiler())->getGraphvizAttributes();

		output << "];" << endl;
	}

	output << nodeCommentSectionDivider;

	// Print comments
	for (vector<VuoComment *>::iterator i = comments.begin(); i != comments.end(); ++i)
	{
		string commentDeclaration = ((*i)->hasCompiler() ?
									  (*i)->getCompiler()->getGraphvizDeclaration(xPositionOffset, yPositionOffset) : "");
		output << commentDeclaration << endl;
	}

	output << commentCableSectionDivider;

	// Print cables
	for (vector<VuoCable *>::iterator i = cables.begin(); i != cables.end(); ++i)
	{
		VuoCable *cable = *i;
		output << cable->getCompiler()->getGraphvizDeclaration() << endl;
	}

	output << "}";
	output << compositionFooter;

	return output.str();
}

/**
 * Returns the set of targets (platforms, operating system versions, architectures) with which this composition is compatible.
 *
 * @throw VuoCompilerException One or more nodes have an unknown node class.
 */
VuoCompilerCompatibility VuoCompilerComposition::getCompatibleTargets()
{
	VuoCompilerIssues *issues = new VuoCompilerIssues;
	checkForMissingNodeClasses(issues);

	VuoCompilerCompatibility compositeTarget(nullptr);
	for (auto node : getBase()->getNodes())
		compositeTarget = compositeTarget.intersection( node->getNodeClass()->getCompiler()->getCompatibleTargets() );

	return compositeTarget;
}

/**
 * Returns true if @c lhs precedes @c rhs in lexicographic order of their identifiers in a .vuo file.
 */
bool VuoCompilerComposition::compareGraphvizIdentifiersOfNodes(VuoNode *lhs, VuoNode *rhs)
{
	string lhsIdentifier = (lhs->hasCompiler() ? lhs->getCompiler()->getGraphvizIdentifier() : lhs->getRawGraphvizIdentifier());
	string rhsIdentifier = (rhs->hasCompiler() ? rhs->getCompiler()->getGraphvizIdentifier() : rhs->getRawGraphvizIdentifier());
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

/**
 * Returns true if @c lhs precedes @c rhs in lexicographic order of their identifiers in a .vuo file.
 */
bool VuoCompilerComposition::compareGraphvizIdentifiersOfComments(VuoComment *lhs, VuoComment *rhs)
{
	string lhsIdentifier = (lhs->hasCompiler() ? lhs->getCompiler()->getGraphvizIdentifier() : "");
	string rhsIdentifier = (rhs->hasCompiler() ? rhs->getCompiler()->getGraphvizIdentifier() : "");
	return lhsIdentifier.compare(rhsIdentifier) < 0;
}

/**
 * Returns true if a port on an old node corresponds to a port on a new node replacing the old one
 * (same name, same data type).
 */
bool VuoCompilerComposition::portsMatch(VuoPort *oldPort, VuoPort *newPort)
{
	if (! oldPort || ! newPort)
		return false;

	string oldPortName = oldPort->getClass()->getName();
	string newPortName = newPort->getClass()->getName();
	if (oldPortName == newPortName)
	{
		if (oldPort->hasCompiler() && newPort->hasCompiler())
		{
			VuoType *oldType = static_cast<VuoCompilerPort *>( oldPort->getCompiler() )->getDataVuoType();
			VuoType *newType = static_cast<VuoCompilerPort *>( newPort->getCompiler() )->getDataVuoType();
			if (oldType == newType)
				return true;
		}
		else
			return true;
	}

	return false;
}

/**
 * Returns true if a port on an old node class corresponds to a port on a new node class replacing the old one
 * (same name, same data type).
 */
bool VuoCompilerComposition::portClassesMatch(VuoPortClass *oldPortClass, VuoPortClass *newPortClass)
{
	if (! oldPortClass || ! newPortClass)
		return false;

	string oldPortName = oldPortClass->getName();
	string newPortName = newPortClass->getName();
	if (oldPortName == newPortName)
	{
		if (oldPortClass->hasCompiler() && newPortClass->hasCompiler())
		{
			VuoType *oldType = static_cast<VuoCompilerPortClass *>( oldPortClass->getCompiler() )->getDataVuoType();
			VuoType *newType = static_cast<VuoCompilerPortClass *>( newPortClass->getCompiler() )->getDataVuoType();
			if (oldType == newType)
				return true;
		}
		else
			return true;
	}

	return false;
}
