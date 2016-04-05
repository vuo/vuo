/**
 * @file
 * VuoCompilerGraphvizParser implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
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
#include "VuoCompilerPublishedInputNodeClass.hh"
#include "VuoCompilerPublishedInputPort.hh"
#include "VuoCompilerPublishedOutputPort.hh"

#include "VuoPort.hh"
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
				VLog("Error: Composition has more than one node of class '%s'.", VuoNodeClass::publishedInputNodeClassName.c_str());
			else if (nodeClassName == VuoNodeClass::publishedOutputNodeClassName)
				VLog("Error: Composition has more than one node of class '%s'.", VuoNodeClass::publishedOutputNodeClassName.c_str());
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
		else if (dummyNodeClassName == VuoNodeClass::publishedInputNodeClassName)
		{
			nodeClassForName[dummyNodeClassName] = compiler->createPublishedInputNodeClass(dummyNodeClass)->getBase();
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
		if (!(pos && sscanf(pos,"%lf,%lf",&x,&y) == 2))
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
			VLog("Error: More than one node with name '%s'.", nodeName.c_str());
			return;
		}

		VuoNode *node;
		if (nodeClass->hasCompiler())
			node = compiler->createNode(nodeClass->getCompiler(), nodeTitle, x, y);
		else
			node = nodeClass->newNode(nodeTitle, x, y);

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
 * Create a cable for each Agedge_t in the .vuo file.
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

			VuoPort *toBasePort = toNode->getInputPortWithName(toPortName);
			VuoPort *fromBasePort = fromNode->getOutputPortWithName(fromPortName);
			if (! toBasePort || ! fromBasePort)
				continue;

			VuoCompilerCable *cable = NULL;

			// If dealing with a published output cable, we will need to create an associated VuoCompilerCable
			// even though we don't currently construct a VuoCompilerNode for the published output node.
			if (fromNode->hasCompiler() && toNode->getNodeClass()->getClassName() == VuoNodeClass::publishedOutputNodeClassName)
			{
				VuoCompilerPort *fromCompilerPort = static_cast<VuoCompilerPort *>(fromBasePort->getCompiler());

				cable = new VuoCompilerCable(fromNode->getCompiler(), fromCompilerPort, NULL, NULL);
				cable->getBase()->setTo(toNode, toBasePort);

				publishedOutputCables.push_back(cable->getBase());
			}

			// Otherwise, if there's no node class implementation, don't try to create cables.
			else if (!fromNode->hasCompiler() || !toNode->hasCompiler())
				continue;

			else
			{
				VuoCompilerPort *toPort = static_cast<VuoCompilerPort *>(toBasePort->getCompiler());
				VuoCompilerPort *fromPort = static_cast<VuoCompilerPort *>(fromBasePort->getCompiler());

				cable = new VuoCompilerCable(fromNode->getCompiler(), fromPort, toNode->getCompiler(), toPort);

				if (fromNode->getNodeClass()->getClassName() == VuoNodeClass::publishedInputNodeClassName)
					publishedInputCables.push_back(cable->getBase());
				else
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
 * Creates a published input port for each outgoing edge of the node of class vuo.in,
 * and a published output port for each incoming edge of the node of class vuo.out.
 */
void VuoCompilerGraphvizParser::makePublishedPorts(void)
{
	for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n))
	{
		string nodeClassName = agget(n, (char *)"type");
		bool isPublishedInputNode = (nodeClassName == VuoNodeClass::publishedInputNodeClassName);
		bool isPublishedOutputNode = (nodeClassName == VuoNodeClass::publishedOutputNodeClassName);
		if (!isPublishedInputNode && !isPublishedOutputNode)
			continue;

		string nodeName = n->name;
		VuoNode *node = nodeForName[nodeName];

		map<string, set<VuoCompilerPort *> > connectedPortsForPublishedInputName;
		map<string, set<VuoCompilerPort *> > connectedPortsForPublishedOutputName;

		vector<string> publishedInputPortNames;
		vector<string> publishedOutputPortNames;

		if (isPublishedInputNode)
		{
			vector<VuoPort *> ports = node->getOutputPorts();
			for (vector<VuoPort *>::iterator i = ports.begin(); i != ports.end(); ++i)
			{
				VuoPort *port = *i;
				string portName = port->getClass()->getName();
				if (portName != VuoNodeClass::publishedInputNodeSimultaneousTriggerName)
				{
					publishedInputPortNames.push_back(portName);
					connectedPortsForPublishedInputName[portName] = set<VuoCompilerPort *>();
				}
			}
		}

		else if (isPublishedOutputNode)
		{
			vector<VuoPort *> ports = node->getInputPorts();
			for (vector<VuoPort *>::iterator i = ports.begin(); i != ports.end(); ++i)
			{
				VuoPort *port = *i;
				string portName = port->getClass()->getName();
				if (portName != "refresh")
				{
					publishedOutputPortNames.push_back(portName);
					connectedPortsForPublishedOutputName[portName] = set<VuoCompilerPort *>();
				}
			}
		}

		for (Agedge_t *e = agfstedge(graph, n); e; e = agnxtedge(graph, e, n))
		{
			string fromNodeName = e->tail->name;
			string toNodeName = e->head->name;
			string fromPortName = e->u.tail_port.name;
			string toPortName = e->u.head_port.name;

			VuoNode *fromNode = nodeForName[fromNodeName];
			VuoNode *toNode = nodeForName[toNodeName];

			VuoPort *toBasePort = toNode->getInputPortWithName(toPortName);
			VuoPort *fromBasePort = fromNode->getOutputPortWithName(fromPortName);
			if (! toBasePort || ! fromBasePort)
				continue;

			if (isPublishedInputNode && toNode->hasCompiler())
			{
				// The edge is from the vuo.in node to some other node.
				// Since there may be multiple out-edges from this port of the vuo.in node, save them for later processing.
				VuoCompilerPort *connectedPort = static_cast<VuoCompilerPort *>(toBasePort->getCompiler());
				connectedPortsForPublishedInputName[fromPortName].insert(connectedPort);
			}
			else if (isPublishedOutputNode && fromNode->hasCompiler())
			{
				// The edge is from the some other node to the vuo.out node.
				// Since there may be multiple out-edges from this port of the vuo.in node, save them for later processing.
				VuoCompilerPort *connectedPort = static_cast<VuoCompilerPort *>(fromBasePort->getCompiler());
				connectedPortsForPublishedOutputName[toPortName].insert(connectedPort);
			}
		}

		map<string, VuoCompilerPublishedPort *> publishedPortForPublishedInputName;
		map<string, VuoCompilerPublishedPort *> publishedPortForPublishedOutputName;

		if (isPublishedInputNode)
		{
			for (map<string, set<VuoCompilerPort *> >::iterator i = connectedPortsForPublishedInputName.begin(); i != connectedPortsForPublishedInputName.end(); ++i)
			{
				string publishedInputName = i->first;
				set<VuoCompilerPort *> connectedPorts = i->second;

				// Parse the data type associated with the published port.
				VuoType *publishedInputType = NULL;
				string publishedPortTypeStr = "";
				parseAttributeOfPort(n, publishedInputName, "type", publishedPortTypeStr);
				if (!publishedPortTypeStr.empty())
					publishedInputType = (publishedPortTypeStr == "event"? NULL :
																			(compiler->getType(publishedPortTypeStr)? compiler->getType(publishedPortTypeStr)->getBase() :
																													  NULL));
				else
					publishedInputType = inferTypeForPublishedPort(publishedInputName, connectedPorts);

				VuoCompilerNodeArgument *fromPort = publishedInputNode->getOutputPortWithName(publishedInputName)->getCompiler();
				VuoCompilerTriggerPort *fromTrigger = static_cast<VuoCompilerTriggerPort *>(fromPort);

				// Create the VuoCompilerPublishedPort from the saved information for the vuo.in node.
				VuoCompilerPublishedPort *port = new VuoCompilerPublishedInputPort(publishedInputName, publishedInputType, connectedPorts, fromTrigger);
				publishedPortForPublishedInputName[publishedInputName] = port;
			}

			// Preserve the original ordering of the published input ports.
			for (vector<string>::iterator i = publishedInputPortNames.begin(); i != publishedInputPortNames.end(); ++i)
				publishedInputPorts.push_back(publishedPortForPublishedInputName[*i]);
		}

		else if (isPublishedOutputNode)
		{
			for (map<string, set<VuoCompilerPort *> >::iterator i = connectedPortsForPublishedOutputName.begin(); i != connectedPortsForPublishedOutputName.end(); ++i)
			{
				string publishedOutputName = i->first;
				set<VuoCompilerPort *> connectedPorts = i->second;

				// Parse the data type associated with the published port.
				VuoType *publishedOutputType = NULL;
				string publishedPortTypeStr = "";
				parseAttributeOfPort(n, publishedOutputName, "type", publishedPortTypeStr);
				if (!publishedPortTypeStr.empty())
					publishedOutputType = (publishedPortTypeStr == "event"? NULL :
																			(compiler->getType(publishedPortTypeStr)? compiler->getType(publishedPortTypeStr)->getBase() :
																													  NULL));
				else
					publishedOutputType = inferTypeForPublishedPort(publishedOutputName, connectedPorts);


				VuoPort *vuoOutPort = publishedOutputNode->getInputPortWithName(publishedOutputName);
				VuoCompilerPublishedPort *port = new VuoCompilerPublishedOutputPort(publishedOutputName, publishedOutputType, connectedPorts, vuoOutPort);
				publishedPortForPublishedOutputName[publishedOutputName] = port;
			}

			// Preserve the original ordering of the published output ports.
			for (vector<string>::iterator i = publishedOutputPortNames.begin(); i != publishedOutputPortNames.end(); ++i)
				publishedOutputPorts.push_back(publishedPortForPublishedOutputName[*i]);
		}
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

	// Find the internal input ports connected to each published input port.
	map<VuoPort *, VuoCompilerPublishedPort *> publishedInputDataPortForInputPort;
	for (vector<VuoCompilerPublishedPort *>::iterator i = publishedInputPorts.begin(); i != publishedInputPorts.end(); ++i)
	{
		VuoCompilerPublishedInputPort *publishedInputPort = static_cast<VuoCompilerPublishedInputPort *>(*i);

		// A given internal input port may have multiple connected published input ports, but at most
		// one that transmits data to the port.  That's the one whose constant value we need.
		if (publishedInputPort->getBase()->getType())
		{
			set<VuoPort *> connectedPorts = publishedInputPort->getBase()->getConnectedPorts();
			for (set<VuoPort *>::iterator j = connectedPorts.begin(); j != connectedPorts.end(); ++j)
			{
				VuoPort *inputPort = *j;

				for (vector<VuoCable *>::iterator k = publishedInputCables.begin(); k != publishedInputCables.end(); ++k)
				{
					VuoCable *publishedInputCable = *k;
					if (publishedInputCable->getFromPort() == publishedInputPort->getVuoPseudoPort() &&
							publishedInputCable->getToPort() == inputPort)
					{
						if (publishedInputCable->getCompiler()->carriesData())
							publishedInputDataPortForInputPort[inputPort] = publishedInputPort;
						break;
					}
				}
			}

			// Set the constant value in association with the published input port itself.
			map<string, string>::iterator constantIter = constantForPublishedInputPort.find(publishedInputPort->getBase()->getName());
			bool hasConstant = (constantIter != constantForPublishedInputPort.end());
			if (hasConstant)
			{
				string constant = constantIter->second;
				publishedInputPort->setInitialValue(constant);
			}
		}
	}

	// Set the constant value of each internal input port.
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
			bool hasConstant = false;
			map<string, string>::iterator constantIter;

			// If the input port is published, use the published input port's constant value.
			VuoCompilerPublishedPort *publishedInputPort = publishedInputDataPortForInputPort[inputPort];
			if (publishedInputPort)
			{
				constantIter = constantForPublishedInputPort.find(publishedInputPort->getBase()->getName());
				hasConstant = (constantIter != constantForPublishedInputPort.end());
			}

			// Otherwise, use the internal input port's constant value.
			else
			{
				constantIter = constantForInputPort.find(inputPort->getClass()->getName());
				hasConstant = (constantIter != constantForInputPort.end());
			}

			if (hasConstant)
			{
				string constant = constantIter->second;

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

			for (vector<VuoCompilerPublishedPort *>::iterator i = publishedInputPorts.begin(); i != publishedInputPorts.end(); ++i)
			{
				VuoCompilerPublishedPort *publishedPort = *i;

				for (vector<string>::iterator j = detailKeys.begin(); j != detailKeys.end(); ++j)
				{
					string detailKey = *j;
					string detailValue;
					bool foundAttribute = parseAttributeOfPort(n, publishedPort->getBase()->getName(), detailKey, detailValue);
					if (foundAttribute)
						publishedPort->setDetail(detailKey, detailValue);
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
			VLog("Error: Couldn't find node %s's port '%s'.", nodeClassName.c_str(), dummyName.c_str());
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
vector<VuoCompilerPublishedPort *> VuoCompilerGraphvizParser::getPublishedInputPorts(void)
{
	return publishedInputPorts;
}

/**
 * Returns a consistently-ordered list of all published output ports in this composition.
 */
vector<VuoCompilerPublishedPort *> VuoCompilerGraphvizParser::getPublishedOutputPorts(void)
{
	return publishedOutputPorts;
}

/**
 * Returns the pseudo-node of class "vuo.in" in the composition, or null if the composition does not have one.
 */
VuoNode * VuoCompilerGraphvizParser::getPublishedInputNode(void)
{
	return publishedInputNode;
}

/**
 * Returns the pseudo-node of class "vuo.out" in the composition, or null if the composition does not have one.
 */
VuoNode * VuoCompilerGraphvizParser::getPublishedOutputNode(void)
{
	return publishedOutputNode;
}

/**
 * Returns the pseudo-cables attached to the output ports of the published input psuedo-node, if any.
 */
vector<VuoCable *> VuoCompilerGraphvizParser::getPublishedInputCables(void)
{
	return publishedInputCables;
}

/**
 * Returns the pseudo-cables attached to the input ports of the published output psuedo-node, if any.
 */
vector<VuoCable *> VuoCompilerGraphvizParser::getPublishedOutputCables(void)
{
	return publishedOutputCables;
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

