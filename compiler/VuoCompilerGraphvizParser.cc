/**
 * @file
 * VuoCompilerGraphvizParser implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>
#include <stdlib.h>
#include <graphviz/gvc.h>
#include <graphviz/types.h>

#include "VuoCompiler.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerGraphvizParser.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerMakeListNodeClass.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerPublishedPortClass.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerType.hh"

#include "VuoCable.hh"
#include "VuoComposition.hh"
#include "VuoNode.hh"
#include "VuoPort.hh"
#include "VuoPublishedPort.hh"
#include "VuoStringUtilities.hh"

#include <graphviz/gvplugin.h>

extern gvplugin_library_t gvplugin_dot_layout_LTX_library; ///< Reference to the statically-built Graphviz Dot library.
extern gvplugin_library_t gvplugin_core_LTX_library; ///< Reference to the statically-built Graphviz core library.

/// Graphviz plugins
lt_symlist_t lt_preloaded_symbols[] =
{
	{ "gvplugin_dot_layout_LTX_library", &gvplugin_dot_layout_LTX_library},
	{ "gvplugin_core_LTX_library", &gvplugin_core_LTX_library},
	{ 0, 0}
};

/**
 * Parse the .vuo file at @c path, using the node classes provided by the compiler.
 */
VuoCompilerGraphvizParser * VuoCompilerGraphvizParser::newParserFromCompositionFile(string path, VuoCompiler *compiler)
{
	string composition = VuoFileUtilities::readFileToString(path);
	return new VuoCompilerGraphvizParser(composition, compiler);
}

/**
 * Parse a .vuo @c file, using the node classes provided by the compiler.
 */
VuoCompilerGraphvizParser * VuoCompilerGraphvizParser::newParserFromCompositionString(const string &composition, VuoCompiler *compiler)
{
	return new VuoCompilerGraphvizParser(composition, compiler);
}

static std::string VuoCompilerGraphvizParser_lastError;	///< The most recent error from Graphviz. Set this to emptystring before calling into Graphviz.

/**
 * Graphviz callback that appends error messages to @ref VuoCompilerGraphvizParser_lastError.
 */
static int VuoCompilerGraphvizParser_error(char *message)
{
	VuoCompilerGraphvizParser_lastError += message;
	return 0;
}

/**
 * Helper function for VuoCompilerGraphvizParser constructors.
 * Parse a .vuo @c file, using the node classes provided by the compiler.
 *
 * @throw VuoCompilerException Couldn't parse the composition.
 */
VuoCompilerGraphvizParser::VuoCompilerGraphvizParser(const string &compositionAsString, VuoCompiler *compiler)
{
	this->compiler = compiler;
	publishedInputNode = NULL;
	publishedOutputNode = NULL;

	agseterrf(VuoCompilerGraphvizParser_error);
	VuoCompilerGraphvizParser_lastError = "";

	// Use builtin Graphviz plugins, not demand-loaded plugins.
	bool demandLoading = false;
	GVC_t *context = gvContextPlugins(lt_preloaded_symbols, demandLoading);

	graph = agmemread((char *)compositionAsString.c_str());
	if (!graph)
	{
		vector<VuoCompilerError> errors;
		VuoCompilerError error("Couldn't parse the composition", VuoCompilerGraphvizParser_lastError, set<VuoNode *>(), set<VuoCable *>());
		errors.push_back(error);
		throw VuoCompilerException(errors);
	}
	agraphattr(graph, (char *)"rankdir", (char *)"LR");
	agraphattr(graph, (char *)"ranksep", (char *)"0.75");
	agnodeattr(graph, (char *)"fontsize", (char *)"18");
	agnodeattr(graph, (char *)"shape", (char *)"Mrecord");
	gvLayout(context, graph, "dot");  // without this, port names are NULL

	makeDummyNodeClasses();
	makeNodeClasses();
	makeNodes();
	makeCables();
	makePublishedPorts();
	setInputPortConstantValues();
	setPublishedPortDetails();
	setTriggerPortEventThrottling();
	saveNodeDeclarations(compositionAsString);

	gvFreeLayout(context, graph);
	agclose(graph);
	gvFreeContext(context);

	VuoComposition::parseHeader(compositionAsString, name, description, copyright);
}

/**
 * Create a dummy node class for each node class name in the .vuo file.
 */
void VuoCompilerGraphvizParser::makeDummyNodeClasses(void)
{
	map<string, bool> nodeClassNamesSeen;
	for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n))
	{
		string nodeClassName = agget(n, (char *)"type");

		if (nodeClassNamesSeen[nodeClassName])
		{
			if (nodeClassName == VuoNodeClass::publishedInputNodeClassName)
				VUserLog("Error: Composition has more than one node of class '%s'.", VuoNodeClass::publishedInputNodeClassName.c_str());
			else if (nodeClassName == VuoNodeClass::publishedOutputNodeClassName)
				VUserLog("Error: Composition has more than one node of class '%s'.", VuoNodeClass::publishedOutputNodeClassName.c_str());
			else
				continue;  // node class already created
		}

		// Add ports listed in the node instance's label.
		vector<string> inputPortClassNames;
		vector<string> outputPortClassNames;
		{
			field_t *nodeInfo = (field_t *)ND_shape_info(n);
			int numNodeInfoFields = nodeInfo->n_flds;
			for (int i = 0; i < numNodeInfoFields; i++)
			{
				field_t *nodeInfoField = nodeInfo->fld[i];

				// Skip the node instance's title.
				if (! nodeInfoField->id)
					continue;

				// The port text should end with '\l' or '\r', indicating whether the port is on left or right side of the node.
				char * lr = strchr(nodeInfoField->lp->text, '\\');
				if (! lr)
					continue;

				if (lr[1] == 'l')	// input port
				{
					// Skip the refresh port, which is added by VuoNodeClass's constructor below.
					if (strcmp(nodeInfoField->id,"refresh") == 0)
						continue;

					if (find(inputPortClassNames.begin(), inputPortClassNames.end(), nodeInfoField->id) == inputPortClassNames.end())
						inputPortClassNames.push_back(nodeInfoField->id);
				}
				else	// output port
				{
					if (find(outputPortClassNames.begin(), outputPortClassNames.end(), nodeInfoField->id) == outputPortClassNames.end())
						outputPortClassNames.push_back(nodeInfoField->id);
				}
			}
		}

		VuoNodeClass * nodeClass = new VuoNodeClass(nodeClassName, inputPortClassNames, outputPortClassNames);
		dummyNodeClassForName[nodeClassName] = nodeClass;
	}
}

/**
 * Matches up each dummy node class with a node class from the compiler.
 */
void VuoCompilerGraphvizParser::makeNodeClasses(void)
{
	for (map<string, VuoNodeClass *>::iterator i = dummyNodeClassForName.begin(), e = dummyNodeClassForName.end(); i != e; ++i)
	{
		string dummyNodeClassName = i->first;
		VuoNodeClass *dummyNodeClass = i->second;

		VuoCompilerNodeClass *nodeClass = NULL;
		if (compiler)
			nodeClass = compiler->getNodeClass(dummyNodeClassName);

		if (nodeClass)
		{
			nodeClassForName[dummyNodeClassName] = nodeClass->getBase();

			checkPortClasses(dummyNodeClassName, dummyNodeClass->getInputPortClasses(), nodeClass->getBase()->getInputPortClasses());
			checkPortClasses(dummyNodeClassName, dummyNodeClass->getOutputPortClasses(), nodeClass->getBase()->getOutputPortClasses());
		}
		else
		{
			nodeClassForName[dummyNodeClassName] = dummyNodeClass;
		}
	}
}

/**
 * Create a node instance for each node instance name in the .vuo file.
 */
void VuoCompilerGraphvizParser::makeNodes(void)
{
	for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n))
	{
		double x,y;
		char * pos = agget(n, (char *)"pos");
		if (!(pos && sscanf(pos,"%20lf,%20lf",&x,&y) == 2))
		{
			// If the 'pos' attribute is unspecified or invalid, use the post-dot-layout coordinates.
			x = ND_coord(n).x;
			// Flip origin from bottom-left to top-left, to match Qt's origin.
			y = GD_bb(graph).UR.y - ND_coord(n).y;
		}

		string nodeClassName = agget(n, (char *)"type");
		string nodeName = n->name;

		string nodeTitle;
		field_t *nodeInfo = (field_t *)ND_shape_info(n);
		int numNodeInfoFields = nodeInfo->n_flds;
		for (int i = 0; i < numNodeInfoFields; i++)
		{
			field_t *nodeInfoField = nodeInfo->fld[i];
			if (! nodeInfoField->id)  // title, as opposed to a port
				nodeTitle = nodeInfoField->lp->text;
		}

		VuoNodeClass *nodeClass = nodeClassForName[nodeClassName];

		if (nodeForName[nodeName])
		{
			VUserLog("Error: More than one node with name '%s'.", nodeName.c_str());
			return;
		}

		VuoNode *node;
		if (nodeClass->hasCompiler())
			node = compiler->createNode(nodeClass->getCompiler(), nodeTitle, x, y);
		else
		{
			node = nodeClass->newNode(nodeTitle, x, y);

			// Also use this node's display title as the default title for the
			// uninstalled node class as a whole, for use in node panel documentation.
			// @todo https://b33p.net/kosada/node/9495 : Display the node-specific title
			// in the panel, not an educated guess at the node class default title.
			nodeClass->setDefaultTitle(nodeTitle);
		}

		char * nodeTintColor = agget(n, (char *)"fillcolor");
		if (nodeTintColor)
		{
			if (strcmp(nodeTintColor, "yellow")==0)
				node->setTintColor(VuoNode::TintYellow);
			else if (strcmp(nodeTintColor, "tangerine")==0)
				node->setTintColor(VuoNode::TintTangerine);
			else if (strcmp(nodeTintColor, "orange")==0)
				node->setTintColor(VuoNode::TintOrange);
			else if (strcmp(nodeTintColor, "magenta")==0)
				node->setTintColor(VuoNode::TintMagenta);
			else if (strcmp(nodeTintColor, "violet")==0)
				node->setTintColor(VuoNode::TintViolet);
			else if (strcmp(nodeTintColor, "blue")==0)
				node->setTintColor(VuoNode::TintBlue);
			else if (strcmp(nodeTintColor, "cyan")==0)
				node->setTintColor(VuoNode::TintCyan);
			else if (strcmp(nodeTintColor, "green")==0)
				node->setTintColor(VuoNode::TintGreen);
			else if (strcmp(nodeTintColor, "lime")==0)
				node->setTintColor(VuoNode::TintLime);
		}

		char *nodeCollapsed = agget(n, (char *)"collapsed");
		if (nodeCollapsed && strcmp(nodeCollapsed, "true") == 0)
			node->setCollapsed(true);

		nodeForName[nodeName] = node;
		if (nodeClass->hasCompiler())
			node->getCompiler()->setGraphvizIdentifier(nodeName);

		if (nodeClass->getClassName() == VuoNodeClass::publishedInputNodeClassName)
			publishedInputNode = node;
		else if (nodeClass->getClassName() == VuoNodeClass::publishedOutputNodeClassName)
			publishedOutputNode = node;
		else
			orderedNodes.push_back(node);
	}
}

/**
 * Creates a cable for each Agedge_t in the .vuo file representing a non-published cable,
 * and a placeholder for each one representing a published cable.
 */
void VuoCompilerGraphvizParser::makeCables(void)
{
	map<string, bool> nodeNamesSeen;
	for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n))
	{
		for (Agedge_t *e = agfstedge(graph, n); e; e = agnxtedge(graph, e, n))
		{
			string fromNodeName = e->tail->name;
			string toNodeName = e->head->name;
			string fromPortName = e->u.tail_port.name;
			string toPortName = e->u.head_port.name;

			if (nodeNamesSeen[fromNodeName] || nodeNamesSeen[toNodeName])
				continue;  // edge N1 -> N2 appears in N1's edge list and in N2's edge list

			VuoNode *fromNode = nodeForName[fromNodeName];
			VuoNode *toNode = nodeForName[toNodeName];

			VuoPort *toPort = toNode->getInputPortWithName(toPortName);
			VuoPort *fromPort = fromNode->getOutputPortWithName(fromPortName);
			if (! toPort || ! fromPort)
				continue;

			VuoCompilerNode *fromCompilerNode = NULL;
			VuoCompilerPort *fromCompilerPort = NULL;
			if (fromNode->hasCompiler())
			{
				fromCompilerNode = fromNode->getCompiler();
				fromCompilerPort = static_cast<VuoCompilerPort *>(fromPort->getCompiler());
			}
			else if (fromNode != publishedInputNode)
				continue;

			VuoCompilerNode *toCompilerNode = NULL;
			VuoCompilerPort *toCompilerPort = NULL;
			if (toNode->hasCompiler())
			{
				toCompilerNode = toNode->getCompiler();
				toCompilerPort = static_cast<VuoCompilerPort *>(toPort->getCompiler());
			}
			else if (toNode != publishedOutputNode)
				continue;

			VuoCompilerCable *cable = new VuoCompilerCable(fromCompilerNode, fromCompilerPort, toCompilerNode, toCompilerPort);
			if (fromNode == publishedInputNode || toNode == publishedOutputNode)
			{
				publishedCablesInProgress[orderedCables.size()] = make_pair(cable, make_pair(fromPortName, toPortName));
				orderedCables.push_back(NULL);
			}
			else
			{
				orderedCables.push_back(cable->getBase());
			}

			char *eventOnlyAttribute = agget(e, (char *)"event");
			if (eventOnlyAttribute && strcmp(eventOnlyAttribute, "true") == 0)
				cable->setAlwaysEventOnly(true);

			char *hiddenAttribute = agget(e, (char *)"style");
			if (hiddenAttribute && strcmp(hiddenAttribute, "invis") == 0)
				cable->setHidden(true);
		}

		nodeNamesSeen[n->name] = true;
	}
}

/**
 * Creates a published input port for each outgoing edge of the pubished input node,
 * and a published output port for each incoming edge of the published output node.
 * Replaces the placeholder cables from makeCables() with actual cables.
 */
void VuoCompilerGraphvizParser::makePublishedPorts(void)
{
	map<string, set<VuoCompilerPort *> > connectedPortsForPublishedInputPort;
	map<string, set<VuoCompilerPort *> > connectedPortsForPublishedOutputPort;
	for (map< size_t, pair< VuoCompilerCable *, pair<string, string> > >::iterator i = publishedCablesInProgress.begin(); i != publishedCablesInProgress.end(); ++i)
	{
		VuoCompilerCable *cable = i->second.first;
		string fromPortName = i->second.second.first;
		string toPortName = i->second.second.second;

		if (cable->getBase()->getFromNode() == NULL && cable->getBase()->getToPort() != NULL)
		{
			VuoCompilerPort *connectedPort = static_cast<VuoCompilerPort *>(cable->getBase()->getToPort()->getCompiler());
			connectedPortsForPublishedInputPort[fromPortName].insert(connectedPort);
		}
		else if (cable->getBase()->getToNode() == NULL && cable->getBase()->getFromPort() != NULL)
		{
			VuoCompilerPort *connectedPort = static_cast<VuoCompilerPort *>(cable->getBase()->getFromPort()->getCompiler());
			connectedPortsForPublishedOutputPort[toPortName].insert(connectedPort);
		}
		/// @todo https://b33p.net/kosada/node/7756
	}

	map<string, VuoType *> typeForPublishedInputPort;
	map<string, VuoType *> typeForPublishedOutputPort;
	for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n))
	{
		string nodeName = n->name;
		VuoNode *node = nodeForName[nodeName];
		if (node != publishedInputNode && node != publishedOutputNode)
			continue;

		vector<VuoPort *> publishedPorts = (node == publishedInputNode ? node->getOutputPorts() : node->getInputPorts());
		for (vector<VuoPort *>::iterator i = publishedPorts.begin(); i != publishedPorts.end(); ++i)
		{
			string portName = (*i)->getClass()->getName();
			VuoType *portType = NULL;
			string portTypeStr = "";
			parseAttributeOfPort(n, portName, "type", portTypeStr);
			if (portTypeStr.empty())
			{
				set<VuoCompilerPort *> connectedPorts = (node == publishedInputNode ?
															 connectedPortsForPublishedInputPort[portName] :
															 connectedPortsForPublishedOutputPort[portName]);
				portType = inferTypeForPublishedPort(portName, connectedPorts);
			}
			else if (portTypeStr != "event")
			{
				VuoCompilerType *portCompilerType = compiler->getType(portTypeStr);
				if (portCompilerType)
					portType = portCompilerType->getBase();
			}

			if (node == publishedInputNode)
				typeForPublishedInputPort[portName] = portType;
			else
				typeForPublishedOutputPort[portName] = portType;
		}
	}

	map<string, VuoPort *> publishedInputPortForName;
	map<string, VuoPort *> publishedOutputPortForName;
	for (int i = 0; i < 2; ++i)
	{
		if ((i == 0 && ! publishedInputNode) || (i == 1 && ! publishedOutputNode))
			continue;

		vector<VuoPort *> basePorts;
		int startIndex;
		if (i == 0)
		{
			basePorts = publishedInputNode->getOutputPorts();
			startIndex = VuoNodeClass::unreservedOutputPortStartIndex;
		}
		else
		{
			basePorts = publishedOutputNode->getInputPorts();
			startIndex = VuoNodeClass::unreservedInputPortStartIndex;
		}

		for (int j = startIndex; j < basePorts.size(); ++j)
		{
			string portName = basePorts[j]->getClass()->getName();
			VuoType *vuoType = (i == 0 ? typeForPublishedInputPort[portName] : typeForPublishedOutputPort[portName]);
			Type *llvmType = (vuoType == NULL ? NULL : vuoType->getCompiler()->getType());
			VuoPortClass::PortType eventOrData = (vuoType == NULL ? VuoPortClass::eventOnlyPort : VuoPortClass::dataAndEventPort);
			VuoCompilerPublishedPortClass *portClass = new VuoCompilerPublishedPortClass(portName, eventOrData, llvmType);
			portClass->setDataVuoType(vuoType);
			VuoCompilerPublishedPort *publishedPort = static_cast<VuoCompilerPublishedPort *>( portClass->newPort() );
			if (i == 0)
			{
				publishedInputPortForName[portName] = publishedPort->getBase();
				publishedInputPorts.push_back( static_cast<VuoPublishedPort *>(publishedPort->getBase()) );
			}
			else
			{
				publishedOutputPortForName[portName] = publishedPort->getBase();
				publishedOutputPorts.push_back( static_cast<VuoPublishedPort *>(publishedPort->getBase()) );
			}
		}
	}

	for (map< size_t, pair< VuoCompilerCable *, pair<string, string> > >::iterator i = publishedCablesInProgress.begin(); i != publishedCablesInProgress.end(); ++i)
	{
		size_t index = i->first;
		VuoCompilerCable *cable = i->second.first;
		string fromPortName = i->second.second.first;
		string toPortName = i->second.second.second;

		if (cable->getBase()->getFromNode() == NULL)
			cable->getBase()->setFrom(publishedInputNode, publishedInputPortForName[fromPortName]);
		if (cable->getBase()->getToNode() == NULL)
			cable->getBase()->setTo(publishedOutputNode, publishedOutputPortForName[toPortName]);

		orderedCables[index] = cable->getBase();
	}
}

/**
 * Parses the constant value for each input port that has one.
 */
void VuoCompilerGraphvizParser::setInputPortConstantValues(void)
{
	// Find the constant value of each published input port.
	map<string, string> constantForPublishedInputPort;
	for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n))
		if (nodeForName[n->name] == publishedInputNode)
			constantForPublishedInputPort = parsePortConstantValues(n);

	// Set the constant value of each published input port.
	for (vector<VuoPublishedPort *>::iterator i = publishedInputPorts.begin(); i != publishedInputPorts.end(); ++i)
	{
		VuoPublishedPort *publishedInputPort = *i;

		map<string, string>::iterator constantIter = constantForPublishedInputPort.find( publishedInputPort->getClass()->getName() );
		if (constantIter != constantForPublishedInputPort.end())
			static_cast<VuoCompilerPublishedPort *>( publishedInputPort->getCompiler() )->setInitialValue( constantIter->second );
	}

	// Find and set the constant value of each internal input port.
	for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n))
	{
		VuoNode *node = nodeForName[n->name];
		if (! node->hasCompiler())
			continue;

		map<string, string> constantForInputPort = parsePortConstantValues(n);

		vector<VuoPort *> inputPorts = node->getInputPorts();
		for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
		{
			VuoPort *inputPort = *i;

			VuoPort *publishedInputPort = NULL;
			vector<VuoCable *> connectedCables = inputPort->getConnectedCables();
			for (vector<VuoCable *>::iterator j = connectedCables.begin(); j != connectedCables.end(); ++j)
			{
				VuoCable *cable = *j;

				if (cable->getCompiler()->carriesData())
				{
					if (cable->getFromNode() == publishedInputNode)
						publishedInputPort = cable->getFromPort();
					break;
				}
			}

			bool hasConstant = false;
			string constant;

			if (publishedInputPort)
			{
				// If the input port has an incoming data cable from a published port, use the published port's constant or default value.
				constant = constantForPublishedInputPort[ publishedInputPort->getClass()->getName() ];
				hasConstant = true;
			}
			else
			{
				// Otherwise, use the internal input port's constant value.
				map<string, string>::iterator constantIter = constantForInputPort.find(inputPort->getClass()->getName());
				hasConstant = (constantIter != constantForInputPort.end());
				if (hasConstant)
					constant = constantIter->second;
			}

			if (hasConstant)
			{
				VuoCompilerInputEventPort *inputEventPort = dynamic_cast<VuoCompilerInputEventPort *>(inputPort->getCompiler());
				VuoCompilerInputData *data = inputEventPort->getData();
				if (data)
					data->setInitialValue(constant);
			}
		}
	}
}

/**
 * Parses the details (metadata) for each published port.
 */
void VuoCompilerGraphvizParser::setPublishedPortDetails(void)
{
	for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n))
	{
		string nodeClassName = agget(n, (char *)"type");

		if (nodeClassName == VuoNodeClass::publishedInputNodeClassName)
		{
			vector<string> detailKeys;
			detailKeys.push_back("suggestedMin");
			detailKeys.push_back("suggestedMax");
			detailKeys.push_back("suggestedStep");

			for (vector<VuoPublishedPort *>::iterator i = publishedInputPorts.begin(); i != publishedInputPorts.end(); ++i)
			{
				VuoPublishedPort *publishedPort = *i;

				for (vector<string>::iterator j = detailKeys.begin(); j != detailKeys.end(); ++j)
				{
					string detailKey = *j;
					string detailValue;
					bool foundAttribute = parseAttributeOfPort(n, publishedPort->getClass()->getName(), detailKey, detailValue);
					if (foundAttribute)
					{
						VuoCompilerPublishedPortClass *portClass = static_cast<VuoCompilerPublishedPortClass *>(publishedPort->getClass()->getCompiler());
						portClass->setDetail(detailKey, detailValue);
					}
				}
			}
		}
	}
}

/**
 * Parses the event-throttling attribute for each trigger port.
 */
void VuoCompilerGraphvizParser::setTriggerPortEventThrottling(void)
{
	for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n))
	{
		VuoNode *node = nodeForName[n->name];

		vector<VuoPort *> outputPorts = node->getOutputPorts();
		for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
		{
			VuoPort *port = *i;
			if (port->getClass()->getPortType() == VuoPortClass::triggerPort)
			{
				string eventThrottlingStr;
				parseAttributeOfPort(n, port->getClass()->getName(), "eventThrottling", eventThrottlingStr);
				enum VuoPortClass::EventThrottling eventThrottling;
				if (eventThrottlingStr == "drop")
					eventThrottling = VuoPortClass::EventThrottling_Drop;
				else
					// If composition was created before event dropping was implemented, default to
					// event enqueuing for backward compatibility (preserving the original behavior).
					eventThrottling = VuoPortClass::EventThrottling_Enqueue;
				port->setEventThrottling(eventThrottling);
			}
		}
	}
}

/**
 * Returns a mapping of port names to constant values.
 *
 * A port name only appears in the map if it has a constant value defined in the composition.
 */
map<string, string> VuoCompilerGraphvizParser::parsePortConstantValues(Agnode_t *n)
{
	map<string, string> constantForInputPort;

	field_t *nodeInfo = (field_t *)ND_shape_info(n);
	int numNodeInfoFields = nodeInfo->n_flds;
	for (int i = 0; i < numNodeInfoFields; i++)
	{
		field_t *nodeInfoField = nodeInfo->fld[i];
		char *inputPortName = nodeInfoField->id;
		if (! inputPortName)
			continue;

		string constantValue;
		if (parseAttributeOfPort(n, inputPortName, "", constantValue))
			constantForInputPort[inputPortName] = constantValue;
	}

	return constantForInputPort;
}

/**
 * Parses a port attribute from a node declaration.
 *
 * If the port attribute is found, returns true and sets @a attributeValue. Otherwise, returns false.
 */
bool VuoCompilerGraphvizParser::parseAttributeOfPort(Agnode_t *n, string portName, string suffix, string &attributeValue)
{
	ostringstream oss;
	oss << "_" << portName;
	if (! suffix.empty())
		oss << "_" << suffix;
	char *attributeName = strdup(oss.str().c_str());

	char *rawAttributeValue = agget(n, attributeName);
	free(attributeName);

	// The Graphviz parser may return a constant value of the empty string if a constant value was defined
	// for another identically named port within the same composition, even if it wasn't defined for this port.
	// Any constant value that has been customized within the Vuo Editor should be non-empty anyway.
	// Therefore, treat constants consisting of the empty string as if they were absent so that they
	// don't override node-class-specific defaults.
	if (rawAttributeValue && strcmp(rawAttributeValue, ""))
	{
		attributeValue = VuoStringUtilities::transcodeFromGraphvizIdentifier(rawAttributeValue);
		return true;
	}

	return false;
}

/**
 * Check that the dummy input/output port classes are a subset of the actual input/output port classes.
 */
void VuoCompilerGraphvizParser::checkPortClasses(string nodeClassName, vector<VuoPortClass *> dummy, vector<VuoPortClass *> actual)
{
	for (vector<VuoPortClass *>::iterator i = dummy.begin(); i != dummy.end(); ++i)
	{
		string dummyName = (*i)->getName();

		if (dummyName == "refresh")
			continue;

		bool found = false;
		for (vector<VuoPortClass *>::iterator j = actual.begin(); j != actual.end(); ++j)
		{
			if ((*j)->getName() == dummyName)
			{
				found = true;
				break;
			}
		}
		if (! found)
		{
			VUserLog("Error: Couldn't find node %s's port '%s'.", nodeClassName.c_str(), dummyName.c_str());
			return;
		}
	}
}

/**
 * Stores the Graphviz declaration for each node.
 */
void VuoCompilerGraphvizParser::saveNodeDeclarations(const string &compositionAsString)
{
	size_t nodesRemaining = nodeForName.size();

	vector<string> lines = VuoStringUtilities::split(compositionAsString, '\n');
	for (vector<string>::iterator i = lines.begin(); i != lines.end() && nodesRemaining > 0; ++i)
	{
		string line = *i;
		string identifier;
		for (int j = 0; j < line.length() && VuoStringUtilities::isValidCharInIdentifier(line[j]); ++j)
			identifier += line[j];

		map<string, VuoNode *>::iterator nodeIter = nodeForName.find(identifier);
		if (nodeIter != nodeForName.end())
		{
			VuoNode *node = nodeIter->second;
			node->setRawGraphvizDeclaration(line);
			--nodesRemaining;
		}
	}
}

/**
 * Returns a list of all the nodes in this composition in the order they were listed in the .vuo file,
 * excluding any psuedo-nodes of class vuo.in or vuo.out.
 */
vector<VuoNode *> VuoCompilerGraphvizParser::getNodes(void)
{
	return orderedNodes;
}

/**
 * Returns a list of all the cables in this composition in the order they were listed in the .vuo file,
 * excluding any pseudo-cables connected to pseudo-nodes of class vuo.in or vuo.out.
 */
vector<VuoCable *> VuoCompilerGraphvizParser::getCables(void)
{
	return orderedCables;
}

/**
 * Returns a consistently-ordered list of all published input ports in this composition.
 */
vector<VuoPublishedPort *> VuoCompilerGraphvizParser::getPublishedInputPorts(void)
{
	return publishedInputPorts;
}

/**
 * Returns a consistently-ordered list of all published output ports in this composition.
 */
vector<VuoPublishedPort *> VuoCompilerGraphvizParser::getPublishedOutputPorts(void)
{
	return publishedOutputPorts;
}

/**
 * Returns the composition's title.
 */
string VuoCompilerGraphvizParser::getName(void)
{
	return name;
}

/**
 * Returns the composition's description.
 */
string VuoCompilerGraphvizParser::getDescription(void)
{
	return description;
}

/**
 * Returns the composition's copyright.
 */
string VuoCompilerGraphvizParser::getCopyright(void)
{
	return copyright;
}

/**
 * Infers this published port's data type from its name and connected internal ports.
 * Returns the inferred type, or NULL for event-only.
 */
VuoType * VuoCompilerGraphvizParser::inferTypeForPublishedPort(string name, const set<VuoCompilerPort *> &connectedPorts)
{
	if (connectedPorts.empty() || name == "refresh")
		return NULL;

	VuoCompilerPort *connectedPort = *connectedPorts.begin();
	VuoCompilerPortClass *connectedPortClass = static_cast<VuoCompilerPortClass *>(connectedPort->getBase()->getClass()->getCompiler());
	return connectedPortClass->getDataVuoType();
}

