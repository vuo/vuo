/**
 * @file
 * VuoCompilerGraphvizParser implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <graphviz/gvc.h>
#include <graphviz/types.h>

#include "VuoCompiler.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerGraphvizParser.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerMakeListNodeClass.hh"
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
VuoCompilerGraphvizParser::VuoCompilerGraphvizParser(const string &path, VuoCompiler *compiler)
{
	FILE *file = fopen(path.c_str(), "r");
	if (! file)
	{
		fprintf(stderr, "Couldn't read composition at path %s\n", path.c_str());
		return;
	}

	initialize(file, compiler);
}

/**
 * Parse a .vuo @c file, using the node classes provided by the compiler.
 */
VuoCompilerGraphvizParser::VuoCompilerGraphvizParser(FILE *file, VuoCompiler *compiler)
{
	initialize(file, compiler);
}

/**
 * Helper function for VuoCompilerGraphvizParser constructors.
 * Parse a .vuo @c file, using the node classes provided by the compiler.
 */
void VuoCompilerGraphvizParser::initialize(FILE *file, VuoCompiler *compiler)
{
	this->compiler = compiler;
	publishedInputNode = NULL;
	publishedOutputNode = NULL;

	string graphAsString = VuoFileUtilities::cFileToString(file);

	// Use builtin Graphviz plugins, not demand-loaded plugins.
	bool demandLoading = false;
	GVC_t *context = gvContextPlugins(lt_preloaded_symbols, demandLoading);

	graph = agmemread((char *)graphAsString.c_str());
	agraphattr(graph, (char *)"rankdir", (char *)"LR");
	agraphattr(graph, (char *)"ranksep", (char *)"0.75");
	agnodeattr(graph, (char *)"fontsize", (char *)"18");
	agnodeattr(graph, (char *)"shape", (char *)"Mrecord");
	gvLayout(context, graph, "dot");  // without this, port names are NULL

	makeDummyPorts();
	makeDummyNodeClasses();
	makeNodeClasses();
	makeNodes();
	makeCables();
	makePublishedPorts();
	setInputPortConstantValues();

	gvFreeLayout(context, graph);
	agclose(graph);
	gvFreeContext(context);

	description = parseDescription(graphAsString);
}

/**
 * Find the names of all input and output ports that have cables attached to them.
 */
void VuoCompilerGraphvizParser::makeDummyPorts(void)
{
	map<string, bool> nodeNamesSeen;
	for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n))
	{
		for (Agedge_t *e = agfstedge(graph, n); e; e = agnxtedge(graph, e, n))
		{
			string fromNodeClassName = agget(e->tail, (char *)"type");
			string toNodeClassName = agget(e->head, (char *)"type");
			string fromNodeName = e->tail->name;
			string toNodeName = e->head->name;
			string fromPortName = e->u.tail_port.name;
			string toPortName = e->u.head_port.name;

			if (nodeNamesSeen[fromNodeName] || nodeNamesSeen[toNodeName])
				continue;  // edge N1 -> N2 appears in N1's edge list and in N2's edge list

			dummyOutputNamesForNodeClassName[fromNodeClassName].insert(fromPortName);
			dummyInputNamesForNodeClassName[toNodeClassName].insert(toPortName);
		}

		nodeNamesSeen[n->name] = true;
	}
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
				fprintf(stderr, "Composition has more than one node of class %s\n", VuoNodeClass::publishedInputNodeClassName.c_str());
			else if (nodeClassName == VuoNodeClass::publishedOutputNodeClassName)
				fprintf(stderr, "Composition has more than one node of class %s\n", VuoNodeClass::publishedOutputNodeClassName.c_str());
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
					// Skip the refresh port and done port, which are added by VuoNodeClass's constructor below.
					if (strcmp(nodeInfoField->id,"refresh") == 0 || strcmp(nodeInfoField->id,"done") == 0)
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
 * Match up each dummy node class with a node class from the compiler.
 */
void VuoCompilerGraphvizParser::makeNodeClasses(void)
{
	for (map<string, VuoNodeClass *>::iterator i = dummyNodeClassForName.begin(), e = dummyNodeClassForName.end(); i != e; ++i)
	{
		string dummyNodeClassName = i->first;
		VuoNodeClass *dummyNodeClass = i->second;

		VuoCompilerNodeClass *nodeClass = (compiler ? compiler->getNodeClass(dummyNodeClassName) : NULL);
		if (nodeClass)
		{
			nodeClassForName[dummyNodeClassName] = nodeClass->getBase();

			checkPortClasses(dummyNodeClass->getInputPortClasses(), nodeClass->getBase()->getInputPortClasses());
			checkPortClasses(dummyNodeClass->getOutputPortClasses(), nodeClass->getBase()->getOutputPortClasses());
		}
		else if (dummyNodeClassName == VuoNodeClass::publishedInputNodeClassName)
		{
			nodeClassForName[dummyNodeClassName] = VuoCompilerNodeClass::createPublishedInputNodeClass(dummyNodeClass->getOutputPortClasses());
		}
		else if (dummyNodeClassName == VuoNodeClass::publishedOutputNodeClassName)
		{
			nodeClassForName[dummyNodeClassName] = dummyNodeClass;
		}
		else if (VuoCompilerMakeListNodeClass::isMakeListNodeClassName(dummyNodeClassName))
		{
			if (compiler)
				nodeClassForName[dummyNodeClassName] = VuoCompilerMakeListNodeClass::getNodeClass(dummyNodeClassName, compiler)->getBase();
			else
				nodeClassForName[dummyNodeClassName] = dummyNodeClass;
		}
		else
		{
			nodeClassForName[dummyNodeClassName] = dummyNodeClass;
			unknownNodeClasses.push_back(dummyNodeClassName);
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
			fprintf(stderr, "More than one node with name %s\n", nodeName.c_str());
			return;
		}

		VuoNode *node;
		if (nodeClass->hasCompiler())
			node = nodeClass->getCompiler()->newNode(nodeTitle, x, y);
		else
			node = nodeClass->newNode(nodeTitle, x, y);

		char * nodeTintColor = agget(n, (char *)"fillcolor");
		if (nodeTintColor)
		{
			if (strcmp(nodeTintColor, "yellow")==0)
				node->setTintColor(VuoNode::TintYellow);
			else if (strcmp(nodeTintColor, "orange")==0)
				node->setTintColor(VuoNode::TintOrange);
			else if (strcmp(nodeTintColor, "magenta")==0)
				node->setTintColor(VuoNode::TintMagenta);
			else if (strcmp(nodeTintColor, "violet")==0)
				node->setTintColor(VuoNode::TintViolet);
			else if (strcmp(nodeTintColor, "cyan")==0)
				node->setTintColor(VuoNode::TintCyan);
			else if (strcmp(nodeTintColor, "green")==0)
				node->setTintColor(VuoNode::TintGreen);
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

		else // if (nodeClass->getClassName() != VuoNodeClass::publishedInputNodeClassName && nodeClass->getClassName() != VuoNodeClass::publishedOutputNodeClassName)
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
		string nodeClassName = agget(n, (char *)"type");

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

			// If dealing with a published output cable, we will need to create an associated VuoCompilerCable
			// even though we don't currently construct a VuoCompilerNode for the published output node.
			if (toNode->getNodeClass()->getClassName() == VuoNodeClass::publishedOutputNodeClassName)
			{
				VuoPort *toPort = toNode->getInputPortWithName(toPortName);
				VuoPort *fromPort = fromNode->getOutputPortWithName(fromPortName);
				VuoCompilerPort *fromCompilerPort = static_cast<VuoCompilerPort *>(fromPort->getCompiler());

				VuoCompilerCable *publishedOutputCable = new VuoCompilerCable(fromNode->getCompiler(), fromCompilerPort, NULL, NULL);
				publishedOutputCable->getBase()->setTo(toNode, toPort);

				publishedOutputCables.push_back(publishedOutputCable->getBase());
			}

			// Otherwise, if there's no node class implementation, don't try to create cables.
			else if (!fromNode->hasCompiler() || !toNode->hasCompiler())
				continue;

			else
			{
				VuoCompilerPort *toPort = static_cast<VuoCompilerPort *>(toNode->getInputPortWithName(toPortName)->getCompiler());
				VuoCompilerPort *fromPort = static_cast<VuoCompilerPort *>(fromNode->getOutputPortWithName(fromPortName)->getCompiler());

				VuoCompilerCable *cable = new VuoCompilerCable(fromNode->getCompiler(), fromPort, toNode->getCompiler(), toPort);

				if (fromNode->getNodeClass()->getClassName() == VuoNodeClass::publishedInputNodeClassName)
					publishedInputCables.push_back(cable->getBase());
				else
					orderedCables.push_back(cable->getBase());
			}
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
	map<string, set<VuoCompilerPort *> > connectedPortsForPublishedInputName;

	for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n))
	{
		string nodeClassName = agget(n, (char *)"type");
		if (nodeClassName != VuoNodeClass::publishedInputNodeClassName && nodeClassName != VuoNodeClass::publishedOutputNodeClassName)
			continue;

		for (Agedge_t *e = agfstedge(graph, n); e; e = agnxtedge(graph, e, n))
		{
			string fromNodeName = e->tail->name;
			string toNodeName = e->head->name;
			string fromPortName = e->u.tail_port.name;
			string toPortName = e->u.head_port.name;

			VuoNode *fromNode = nodeForName[fromNodeName];
			VuoNode *toNode = nodeForName[toNodeName];

			if (nodeClassName == VuoNodeClass::publishedInputNodeClassName)
			{
				// The edge is from the vuo.in node to some other node.
				// Since there may be multiple out-edges from this port of the vuo.in node, save them for later processing.
				VuoCompilerPort *connectedPort = static_cast<VuoCompilerPort *>(toNode->getInputPortWithName(toPortName)->getCompiler());
				connectedPortsForPublishedInputName[fromPortName].insert(connectedPort);
			}
			else
			{
				// The edge is from the some other node to the vuo.out node.
				// Since there can only be one in-edge to this port of the vuo.out node, go ahead and create the VuoCompilerPublishedPort.
				VuoCompilerPort *connectedPort = static_cast<VuoCompilerPort *>(fromNode->getOutputPortWithName(fromPortName)->getCompiler());
				VuoPort *vuoOutPort = publishedOutputNode->getInputPortWithName(toPortName);
				VuoCompilerPublishedPort *port = new VuoCompilerPublishedOutputPort(toPortName, connectedPort, vuoOutPort);
				publishedOutputPorts.push_back(port);
			}
		}

		if (nodeClassName == VuoNodeClass::publishedInputNodeClassName)
		{
			for (map<string, set<VuoCompilerPort *> >::iterator i = connectedPortsForPublishedInputName.begin(); i != connectedPortsForPublishedInputName.end(); ++i)
			{
				string publishedInputName = i->first;
				set<VuoCompilerPort *> connectedPorts = i->second;

				VuoCompilerNodeArgument *fromPort = publishedInputNode->getOutputPortWithName(publishedInputName)->getCompiler();
				VuoCompilerTriggerPort *fromTrigger = static_cast<VuoCompilerTriggerPort *>(fromPort);

				// Create the VuoCompilerPublishedPort from the saved information for the vuo.in node.
				VuoCompilerPublishedPort *port = new VuoCompilerPublishedInputPort(publishedInputName, connectedPorts, fromTrigger);
				publishedInputPorts.push_back(port);
			}
		}
	}
}

/**
 * Parses the constant value for each input port that has one.
 */
void VuoCompilerGraphvizParser::setInputPortConstantValues(void)
{
	// Find the internal input port connected to each published input port.
	map<VuoPort *, VuoCompilerPublishedPort *> publishedInputPortForInputPort;
	for (vector<VuoCompilerPublishedPort *>::iterator i = publishedInputPorts.begin(); i != publishedInputPorts.end(); ++i)
	{
		VuoCompilerPublishedPort *publishedInputPort = *i;
		set<VuoPort *> connectedPorts = publishedInputPort->getBase()->getConnectedPorts();
		for (set<VuoPort *>::iterator j = connectedPorts.begin(); j != connectedPorts.end(); ++j)
		{
			VuoPort *inputPort = *j;
			publishedInputPortForInputPort[inputPort] = publishedInputPort;
		}
	}

	// Find the constant value of each published input port.
	map<string, string> constantForPublishedInputPort;
	for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n))
		if (nodeForName[n->name] == publishedInputNode)
			constantForPublishedInputPort = parsePortConstantValues(n);

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
			VuoCompilerPublishedPort *publishedInputPort = publishedInputPortForInputPort[inputPort];
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

		char portNameInDotInitializer[strlen("_")+strlen(inputPortName)+1];
		sprintf(portNameInDotInitializer, "_%s", inputPortName);
		char *constantRaw = agget(n, portNameInDotInitializer);
		if (constantRaw)
			constantForInputPort[inputPortName] = VuoStringUtilities::transcodeFromGraphvizIdentifier(constantRaw);
	}

	return constantForInputPort;
}

/**
 * Check that the dummy input/output port classes are a subset of the actual input/output port classes.
 */
void VuoCompilerGraphvizParser::checkPortClasses(vector<VuoPortClass *> dummy, vector<VuoPortClass *> actual)
{
	for (vector<VuoPortClass *>::iterator i = dummy.begin(); i != dummy.end(); ++i)
	{
		string dummyName = (*i)->getName();

		if (dummyName == "refresh" || dummyName == "done")
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
			fprintf(stderr, "Couldn't find port %s\n", dummyName.c_str());
			return;
		}
	}
}

/**
 * Parses the composition's description from the string.
 *
 * The description is assumed to be in the 3rd line starting at the 3rd character,
 * or at the 4th character if the line has a leading space.
 */
string VuoCompilerGraphvizParser::parseDescription(string compositionAsString)
{
	string description = "";
	int charNum = 0;

	for (int lineNum = 1; lineNum <= 3; ++lineNum)
	{
		string line;

		while (true)
		{
			char c = compositionAsString[charNum++];
			if (c == '\n')
				break;
			if (c == 0)
				return "";
			line += c;
		}

		if ((lineNum == 1 && line.substr(0, 2) != "/*") ||
			(lineNum > 1 && line.substr(0, 2) != "* " && line.substr(0, 3) != " * "))
			break;

		if (lineNum == 3)
		{
			description = line;
			if (description.substr(0, 1) == " ")
				description.erase(0, 1);		// Remove optional leading space.
			description.erase(0, 2);			// Remove "* ".
		}
	}

	return description;
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
 * Returns a list of node classes used by this composition whose implementations don't exist in the compiler's specified node class paths.
 */
vector<string> VuoCompilerGraphvizParser::getUnknownNodeClasses(void)
{
	return unknownNodeClasses;
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
 * Returns the composition's description.
 */
string VuoCompilerGraphvizParser::getDescription(void)
{
	return description;
}
