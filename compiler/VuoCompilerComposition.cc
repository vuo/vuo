/**
 * @file
 * VuoCompilerComposition implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerComposition.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerGenericType.hh"
#include "VuoCompilerPublishedInputPort.hh"
#include "VuoCompilerSpecializedNodeClass.hh"
#include "VuoCompilerType.hh"
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
	graph = NULL;
	module = NULL;

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

		publishedInputNode = parser->getPublishedInputNode();
		publishedOutputNode = parser->getPublishedOutputNode();

		getBase()->setName(parser->getName());
		getBase()->setDescription(parser->getDescription());
		getBase()->setCopyright(parser->getCopyright());

		updateGenericPortTypes();
	}
}

/**
 * Destructor.
 */
VuoCompilerComposition::~VuoCompilerComposition(void)
{
	delete graph;
	delete publishedInputNode;
	delete publishedOutputNode;
	VuoCompiler::deleteModule(module);
}

/**
 * Creates a composition from the Graphviz-formatted string representation of a composition.
 */
VuoCompilerComposition * VuoCompilerComposition::newCompositionFromGraphvizDeclaration(const string &compositionGraphvizDeclaration, VuoCompiler *compiler)
{
	VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionString(compositionGraphvizDeclaration, compiler);
	VuoCompilerComposition *composition = new VuoCompilerComposition(new VuoComposition(), parser);
	delete parser;
	return composition;
}

/**
 * Checks that the composition is valid (able to be compiled).
 *
 * @throw std::exception The composition is invalid.
 */
void VuoCompilerComposition::check(void)
{
	checkForMissingNodeClasses();
	checkFeedback();
}

/**
 * Checks that all of the nodes in the composition have a node class known to the compiler.
 *
 * @throw std::runtime_error One or more nodes have an unknown node class.
 */
void VuoCompilerComposition::checkForMissingNodeClasses(void)
{
	set<string> missingNodeClasses;
	set<string> encounteredPremiumModules = VuoCompiler::getEncounteredPremiumModules();
	set<VuoNode *> nodes = getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		if (! node->getNodeClass()->hasCompiler())
		{
			string nodeClassDescription = node->getNodeClass()->getClassName() + " (" + node->getTitle() + ")";
			if (encounteredPremiumModules.find(node->getNodeClass()->getClassName()) != encounteredPremiumModules.end())
				nodeClassDescription += " [premium node]";
			missingNodeClasses.insert(nodeClassDescription);
		}
	}
	if (! missingNodeClasses.empty())
	{
		vector<string> sortedList(missingNodeClasses.begin(), missingNodeClasses.end());
		sort(sortedList.begin(), sortedList.end());
		string sortedString;
		for (vector<string>::iterator i = sortedList.begin(); i != sortedList.end(); ++i)
			sortedString += "\n" + *i;
		throw std::runtime_error("This composition contains nodes that are neither built in to this version of Vuo nor installed on this computer: " + sortedString);
	}
}

/**
 * Checks that the structure of feedback loops in the composition is valid.
 *
 * @param potentialCables Cables that are not yet in the composition but should be included in the check.
 * @throw VuoCompilerException There is at least one infinite feedback loop or deadlocked feedback loop.
 */
void VuoCompilerComposition::checkFeedback(set<VuoCompilerCable *> potentialCables)
{
	delete graph;
	graph = new VuoCompilerGraph(this, potentialCables);

	graph->checkForInfiniteFeedback();
	graph->checkForDeadlockedFeedback();
}

/**
 * Puts the generic ports in the composition into sets, where all ports in a set have the same generic type.
 *
 * @param useOriginalType If true, considers a port generic if it's currently generic or if it's specialized
 *		from a generic. If false, only looks at ports that are currently generic.
 */
set< set<VuoCompilerPort *> > VuoCompilerComposition::groupGenericPortsByType(bool useOriginalType)
{
	set< set<VuoCompilerPort *> > setsOfConnectedGenericPorts;

	// Group each node's generic ports into sets by generic type.
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

		map<string, set<VuoCompilerPort *> > genericPortsForType;
		for (vector<VuoPort *>::iterator j = ports.begin(); j != ports.end(); ++j)
		{
			VuoCompilerPort *port = static_cast<VuoCompilerPort *>((*j)->getCompiler());
			VuoGenericType *genericType = NULL;
			if (useOriginalType)
			{
				VuoCompilerNodeClass *nodeClass = node->getNodeClass()->getCompiler();
				VuoCompilerSpecializedNodeClass *specializedNodeClass = dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass);
				if (specializedNodeClass)
				{
					VuoPortClass *portClass = port->getBase()->getClass();
					genericType = dynamic_cast<VuoGenericType *>( specializedNodeClass->getOriginalPortType(portClass) );
				}
			}
			else
			{
				genericType = dynamic_cast<VuoGenericType *>(port->getDataVuoType());
			}

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

		if (!(cable->getFromPort() && cable->getToPort()))
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
	set< set<VuoCompilerPort *> > setsOfConnectedGenericPorts = groupGenericPortsByType(false);

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
		set<string> compatibleTypeNames;
		for (set<VuoCompilerPort *>::iterator j = connectedGenericPorts.begin(); j != connectedGenericPorts.end(); ++j)
		{
			VuoCompilerPort *port = *j;
			VuoCompilerPortClass *portClass = static_cast<VuoCompilerPortClass *>(port->getBase()->getClass()->getCompiler());
			VuoGenericType *genericTypeFromPortClass = static_cast<VuoGenericType *>(portClass->getDataVuoType());
			VuoGenericType::Compatibility compatibility;
			set<string> compatibleTypeNamesForPort = genericTypeFromPortClass->getCompatibleSpecializedTypes(compatibility);
			set<string> innermostCompatibleTypeNamesForPort;
			for (set<string>::iterator k = compatibleTypeNamesForPort.begin(); k != compatibleTypeNamesForPort.end(); ++k)
				innermostCompatibleTypeNamesForPort.insert( VuoType::extractInnermostTypeName(*k) );

			if (! innermostCompatibleTypeNamesForPort.empty())
			{
				if (compatibleTypeNames.empty())
					compatibleTypeNames = innermostCompatibleTypeNamesForPort;
				else
				{
					set<string> intersectingCompatibleTypeNames;
					set_intersection(compatibleTypeNames.begin(), compatibleTypeNames.end(),
									 innermostCompatibleTypeNamesForPort.begin(), innermostCompatibleTypeNamesForPort.end(),
									 std::insert_iterator<set<string> >(intersectingCompatibleTypeNames, intersectingCompatibleTypeNames.begin() ));
					compatibleTypeNames = intersectingCompatibleTypeNames;
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
			set<string> compatibleTypeNamesForPort;
			string prefix = (VuoType::isListTypeName(typeNameForPort) ? VuoType::listTypeNamePrefix : "");
			for (set<string>::iterator k = compatibleTypeNames.begin(); k != compatibleTypeNames.end(); ++k)
				compatibleTypeNamesForPort.insert(prefix + *k);

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
 * Returns the set of ports that have the same innermost generic type as the given port.
 *
 * Assumes that updateGenericPortTypes() has been called since any changes affecting the groups/networks
 * of connected generic ports have been made to the composition.
 */
set<VuoPort *> VuoCompilerComposition::getConnectedGenericPorts(VuoPort *port)
{
	set< set<VuoCompilerPort *> > setsOfConnectedGenericPorts = groupGenericPortsByType(false);

	set<VuoCompilerPort *> connectedPorts;
	for (set< set<VuoCompilerPort *> >::iterator i = setsOfConnectedGenericPorts.begin(); i != setsOfConnectedGenericPorts.end(); ++i)
	{
		if ((*i).find(static_cast<VuoCompilerPort *>(port->getCompiler())) != (*i).end())
		{
			connectedPorts = *i;
			break;
		}
	}

	set<VuoPort *> connectedBasePorts;
	for (set<VuoCompilerPort *>::iterator i = connectedPorts.begin(); i != connectedPorts.end(); ++i)
		connectedBasePorts.insert((*i)->getBase());

	return connectedBasePorts;
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
 * Takes ownership of the LLVM module containing the compiled composition.
 */
void VuoCompilerComposition::setModule(Module *module)
{
	this->module = module;
}

/**
 * If the given node has the same Graphviz identifier as some other node currently in the composition,
 * or a node that was previously passed through this function, changes the given node's Graphviz identifier
 * to one that has never been used by this composition.
 */
void VuoCompilerComposition::setUniqueGraphvizIdentifierForNode(VuoNode *node)
{
	if (! node->hasCompiler())
		return;

	set<VuoNode *> nodes = getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
		if (*i != node && (*i)->hasCompiler())
			nodeGraphvizIdentifierUsed[ (*i)->getCompiler()->getGraphvizIdentifier() ] = *i;

	string uniqueIdentifier = node->getCompiler()->getGraphvizIdentifier();
	string prefix = node->getCompiler()->getGraphvizIdentifierPrefix();
	int suffix = 1;
	while (nodeGraphvizIdentifierUsed[uniqueIdentifier] != NULL && nodeGraphvizIdentifierUsed[uniqueIdentifier] != node)
	{
		ostringstream oss;
		oss << ++suffix;
		uniqueIdentifier = prefix + oss.str();
	}

	nodeGraphvizIdentifierUsed[uniqueIdentifier] = node;
	node->getCompiler()->setGraphvizIdentifier(uniqueIdentifier);
}

/**
 * Returns the .vuo (Graphviz dot format) representation of this composition.
 */
string VuoCompilerComposition::getGraphvizDeclaration(string header, string footer)
{
	return getGraphvizDeclarationForComponents(getBase()->getNodes(),
											   getBase()->getCables(),
											   getBase()->getPublishedInputPorts(),
											   getBase()->getPublishedOutputPorts(),
											   header,
											   footer);
}

/**
 * Returns the .vuo (Graphviz dot format) representation of the given nodes and cables in this composition.
 */
string VuoCompilerComposition::getGraphvizDeclarationForComponents(set<VuoNode *> nodeSet,
																   set<VuoCable *> cableSet,
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

	string compositionHeader = (! header.empty()? header : defaultGraphDeclaration);
	string compositionFooter = (! footer.empty()? footer : "\n");

	ostringstream output;
	string nodeCableSectionDivider = (! (cables.empty()
										 && publishedInputPorts.empty()
										 && publishedOutputPorts.empty())?
										 "\n":"");

	// Print header
	output << compositionHeader;
	output << "{" << endl;

	// Print nodes
	for (vector<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		string nodeDeclaration = ((*i)->hasCompiler() ?
									  (*i)->getCompiler()->getGraphvizDeclaration(true, xPositionOffset, yPositionOffset) :
									  (*i)->getRawGraphvizDeclaration());
		output << nodeDeclaration << endl;
	}

	// Print pseudo-nodes for published ports
	if (! publishedInputPorts.empty())
	{
		output << VuoNodeClass::publishedInputNodeIdentifier << " [type=\"" << VuoNodeClass::publishedInputNodeClassName << "\" label=\"" << VuoNodeClass::publishedInputNodeIdentifier;
		for (vector<VuoPublishedPort *>::iterator i = publishedInputPorts.begin(); i != publishedInputPorts.end(); ++i)
			output << "|<" << (*i)->getName() << ">" << (*i)->getName() << "\\r";
		output << "\"";
		for (vector<VuoPublishedPort *>::iterator i = publishedInputPorts.begin(); i != publishedInputPorts.end(); ++i)
			output << (*i)->getCompiler()->getGraphvizAttributes();
		output << "];" << endl;
	}
	if (! publishedOutputPorts.empty())
	{
		output << VuoNodeClass::publishedOutputNodeIdentifier <<  " [type=\"" << VuoNodeClass::publishedOutputNodeClassName << "\" label=\"" << VuoNodeClass::publishedOutputNodeIdentifier;
		for (vector<VuoPublishedPort *>::iterator i = publishedOutputPorts.begin(); i != publishedOutputPorts.end(); ++i)
			output << "|<" << (*i)->getName() << ">" << (*i)->getName() << "\\l";
		output << "\"";

		for (vector<VuoPublishedPort *>::iterator i = publishedOutputPorts.begin(); i != publishedOutputPorts.end(); ++i)
			output << (*i)->getCompiler()->getGraphvizAttributes();

		output << "];" << endl;
	}

	output << nodeCableSectionDivider;

	// Print cables
	for (vector<VuoCable *>::iterator cable = cables.begin(); cable != cables.end(); ++cable)
		output << (*cable)->getCompiler()->getGraphvizDeclaration() << endl;

	// Print pseudo-cables for published ports
	for (vector<VuoPublishedPort *>::iterator publishedPort = publishedInputPorts.begin(); publishedPort != publishedInputPorts.end(); ++publishedPort)
	{
		VuoPort *pseudoPort = (*publishedPort)->getCompiler()->getVuoPseudoPort();

		set<VuoCable *> publishedInputCables = getBase()->getPublishedInputCables();
		for (set<VuoCable *>::iterator publishedCable = publishedInputCables.begin(); publishedCable != publishedInputCables.end(); ++publishedCable)
		{
			if ((*publishedCable)->getFromPort() == pseudoPort &&
					(*publishedCable)->getToNode() && (*publishedCable)->getToNode()->hasCompiler() &&
					(std::find(nodes.begin(), nodes.end(), (*publishedCable)->getToNode()) != nodes.end()))
			{
				output << VuoNodeClass::publishedInputNodeIdentifier << ":" << (*publishedPort)->getName() << " -> "
					   << (*publishedCable)->getToNode()->getCompiler()->getGraphvizIdentifier() << ":"
					   << (*publishedCable)->getToPort()->getClass()->getName();

				if ((*publishedCable)->getCompiler()->getAlwaysEventOnly() && (*publishedPort)->getType() &&
						static_cast<VuoCompilerPort *>( (*publishedCable)->getToPort()->getCompiler() )->getDataVuoType())
					output << " [event=true]";

				output << ";" << endl;
			}
		}
	}
	for (vector<VuoPublishedPort *>::iterator publishedPort = publishedOutputPorts.begin(); publishedPort != publishedOutputPorts.end(); ++publishedPort)
	{
		VuoPort *pseudoPort = (*publishedPort)->getCompiler()->getVuoPseudoPort();

		set<VuoCable *> publishedOutputCables = getBase()->getPublishedOutputCables();
		for (set<VuoCable *>::iterator publishedCable = publishedOutputCables.begin(); publishedCable != publishedOutputCables.end(); ++publishedCable)
		{
			if ((*publishedCable)->getToPort() == pseudoPort &&
					(*publishedCable)->getFromNode() && (*publishedCable)->getFromNode()->hasCompiler() &&
					(std::find(nodes.begin(), nodes.end(), (*publishedCable)->getFromNode()) != nodes.end()))
			{
				output << (*publishedCable)->getFromNode()->getCompiler()->getGraphvizIdentifier() << ":"
					   << (*publishedCable)->getFromPort()->getClass()->getName() << " -> "
					   << VuoNodeClass::publishedOutputNodeIdentifier << ":" << (*publishedPort)->getName();

				if ((*publishedCable)->getCompiler()->getAlwaysEventOnly() && (*publishedPort)->getType() &&
						static_cast<VuoCompilerPort *>( (*publishedCable)->getFromPort()->getCompiler() )->getDataVuoType())
					output << " [event=true]";

				output << ";" << endl;
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
 * The string representation is based on the [JSON Patch](http://tools.ietf.org/html/draft-ietf-appsawg-json-patch-02) format,
 * with some extensions. The key used for each node is its Graphviz identifier. Unlike the example below (spaced for readability),
 * the returned string contains no whitespace.
 *
 * @eg{
 * [
 *   {"add" : "FireOnStart", "value" : {"nodeClass" : "vuo.event.fireOnStart"}},
 *   {"remove" : "Round"}
 * ]
 * }
 */
string VuoCompilerComposition::diffAgainstOlderComposition(string oldCompositionGraphvizDeclaration, VuoCompiler *compiler,
														   const set<NodeReplacement> &nodeReplacements)
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
	for (set<NodeReplacement>::iterator nodeReplacementIter = nodeReplacements.begin(); nodeReplacementIter != nodeReplacements.end(); ++nodeReplacementIter)
	{
		// {   "map" : "<node identifier>",
		//      "to" : "<node identifier>",
		//   "ports" : [
		//			{ "map" : "<port identifier>", "to" : "<port identifier>" },
		//			{ "map" : "<port identifier>", "to" : "<port identifier>" },
		//			... ] }
		json_object *mapObj = json_object_new_object();
		json_object *oldNodeIdentifier = json_object_new_string(nodeReplacementIter->oldNodeIdentifier.c_str());
		json_object_object_add(mapObj, "map", oldNodeIdentifier);
		json_object *newNodeIdentifier = json_object_new_string(nodeReplacementIter->newNodeIdentifier.c_str());
		json_object_object_add(mapObj, "to", newNodeIdentifier);
		json_object *ports = json_object_new_array();
		for (map<string, string>::const_iterator portMapIter = nodeReplacementIter->oldAndNewPortIdentifiers.begin(); portMapIter != nodeReplacementIter->oldAndNewPortIdentifiers.end(); ++portMapIter)
		{
			json_object *portObj = json_object_new_object();
			json_object *oldPortIdentifier = json_object_new_string(portMapIter->first.c_str());
			json_object_object_add(portObj, "map", oldPortIdentifier);
			json_object *newPortIdentifier = json_object_new_string(portMapIter->second.c_str());
			json_object_object_add(portObj, "to", newPortIdentifier);
			json_object_array_add(ports, portObj);
		}
		json_object_object_add(mapObj, "ports", ports);
		json_object_array_add(diff, mapObj);
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
 * Needed so this type can be used in STL containers.
 */
bool operator<(const VuoCompilerComposition::NodeReplacement &lhs, const VuoCompilerComposition::NodeReplacement &rhs)
{
	return (lhs.oldNodeIdentifier != rhs.oldNodeIdentifier ?
										 lhs.oldNodeIdentifier < rhs.oldNodeIdentifier :
										 lhs.newNodeIdentifier < rhs.newNodeIdentifier);
}
