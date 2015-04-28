/**
 * @file
 * VuoRendererComposition implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>

#include "VuoRendererComposition.hh"

#include "VuoCompiler.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerGraphvizParser.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerMakeListNodeClass.hh"
#include "VuoCompilerPublishedInputNodeClass.hh"
#include "VuoCompilerPublishedInputPort.hh"
#include "VuoCompilerPublishedOutputPort.hh"

#include "VuoRendererNode.hh"
#include "VuoRendererMakeListNode.hh"
#include "VuoRendererPort.hh"
#include "VuoRendererCable.hh"
#include "VuoRendererTypecastPort.hh"
#include "VuoRendererSignaler.hh"
#include "VuoRendererColors.hh"
#include "VuoRendererFonts.hh"

#include "VuoPort.hh"

#include <sys/stat.h>

#ifdef MAC
#include <objc/runtime.h>
#endif

/**
 * Creates a canvas upon which nodes and cables can be rendered.
 * The canvas initially contains the nodes, cables, and published ports in the base composition.
 *
 * @param baseComposition The VuoComposition to which the new VuoRendererComposition detail class should be attached.
 * @param renderMissingAsPresent Sets whether node classes without implementations should be rendered as though their implementations are present.  (Useful for prototyping node classes.)
 * @param enableCaching Sets whether item renderings should be cached.
 */
VuoRendererComposition::VuoRendererComposition(VuoComposition *baseComposition, bool renderMissingAsPresent, bool enableCaching)
	: VuoBaseDetail<VuoComposition>("VuoRendererComposition", baseComposition)
{
	getBase()->setRenderer(this);

	VuoRendererFonts::getSharedFonts();  // Load the fonts now to avoid a delay later when rendering the first item in the composition.

	setBackgroundTransparent(false);
	this->renderMissingAsPresent = renderMissingAsPresent;
	this->renderActivity = false;
	this->cachingEnabled = enableCaching;

	parser = NULL;

	signaler = new VuoRendererSignaler();


	// Add any nodes, cables, published ports, and published cables that are already in the base composition.

	set<VuoNode *> nodes = getBase()->getNodes();
	foreach (VuoNode *node, nodes)
		addNodeInCompositionToCanvas(node);

	set<VuoCable *> cables = getBase()->getCables();
	foreach (VuoCable *cable, cables)
		addCableInCompositionToCanvas(cable);

	vector<VuoPublishedPort *> publishedInputPorts = getBase()->getPublishedInputPorts();
	foreach (VuoPublishedPort *publishedPort, publishedInputPorts)
		createRendererForPublishedPortInComposition(publishedPort);

	vector<VuoPublishedPort *> publishedOutputPorts = getBase()->getPublishedOutputPorts();
	foreach (VuoPublishedPort *publishedPort, publishedOutputPorts)
		createRendererForPublishedPortInComposition(publishedPort);

	set<VuoCable *> publishedInputCables = getBase()->getPublishedInputCables();
	foreach (VuoCable *cable, publishedInputCables)
		addCableInCompositionToCanvas(cable);

	set<VuoCable *> publishedOutputCables = getBase()->getPublishedOutputCables();
	foreach (VuoCable *cable, publishedOutputCables)
		addCableInCompositionToCanvas(cable);

	collapseTypecastNodes();
	
	// Now that all renderer components have been created, calculate
	// the final positions of collapsed "Make List" drawers.
	foreach (VuoNode *node, nodes)
		node->getRenderer()->layoutConnectedInputDrawers();
}

/**
 * Sets whether the composition should be rendered with a transparent background.
 */
void VuoRendererComposition::setBackgroundTransparent(bool transparent)
{
	if (transparent)
		setBackgroundBrush(Qt::transparent);
	else
		setBackgroundBrush(VuoRendererColors::getSharedColors()->canvasFill());
}

/**
 * Creates a renderer detail for the base node.
 */
VuoRendererNode * VuoRendererComposition::createRendererNode(VuoNode *baseNode)
{
	VuoRendererNode *rn = NULL;

	if (VuoCompilerMakeListNodeClass::isMakeListNodeClassName(baseNode->getNodeClass()->getClassName()))
		rn = new VuoRendererMakeListNode(baseNode, signaler);

	else
		rn = new VuoRendererNode(baseNode, signaler);

	return rn;
}

/**
 * Adds a node to the canvas and the underlying composition.
 *
 * If the node doesn't have a renderer detail, one is created for it.
 *
 * If a node with the same graphviz identifier as this node is already in the canvas,
 * changes the graphviz identifier of this node to be unique.
 */
void VuoRendererComposition::addNode(VuoNode *n)
{
	getBase()->addNode(n);
	addNodeInCompositionToCanvas(n);
}

/**
 * Adds a node to the canvas. Assumes the node is already in the underlying composition.
 *
 * If the node doesn't have a renderer detail, one is created for it.
 *
 * If a node with the same graphviz identifier as this node is already in the canvas,
 * changes the graphviz identifier of this node to be unique.
 */
void VuoRendererComposition::addNodeInCompositionToCanvas(VuoNode *n)
{
	VuoRendererNode *rn = (n->hasRenderer() ? n->getRenderer() : createRendererNode(n));

	if (renderMissingAsPresent)
		rn->setMissingImplementation(false);

	if (getBase()->hasCompiler())
		getBase()->getCompiler()->setUniqueGraphvizIdentifierForNode(n);

	rn->layoutConnectedInputDrawers();
	addItem(rn);

	rn->setCacheModeForNodeAndPorts(getCurrentDefaultCacheMode());
}

/**
 * Adds a cable to the canvas and the underlying composition.
 *
 * If the cable doesn't have a renderer detail, one is created for it.
 */
void VuoRendererComposition::addCable(VuoCable *c)
{
	getBase()->addCable(c);
	addCableInCompositionToCanvas(c);
}

/**
 * Adds a cable to the canvas. Assumes the cable is already in the underlying composition.
 *
 * If the cable doesn't have a renderer detail, one is created for it.
 */
void VuoRendererComposition::addCableInCompositionToCanvas(VuoCable *c)
{
	VuoRendererCable *rc = (c->hasRenderer() ? c->getRenderer() : new VuoRendererCable(c));

	// Render cables behind nodes.
	rc->setZValue(-1);
	addItem(rc);

	// Performance optimizations
	rc->setCacheMode(getCurrentDefaultCacheMode());
}

/**
 * Adds a published input cable to the canvas and the underlying composition.
 *
 * If the cable doesn't have a renderer detail, one is created for it.
 */
void VuoRendererComposition::addPublishedInputCable(VuoCable *c)
{
	getBase()->addPublishedInputCable(c);
	addCableInCompositionToCanvas(c);
}

/**
 * Adds a published output cable to the canvas and the underlying composition.
 *
 * If the cable doesn't have a renderer detail, one is created for it.
 */
void VuoRendererComposition::addPublishedOutputCable(VuoCable *c)
{
	getBase()->addPublishedOutputCable(c);
	addCableInCompositionToCanvas(c);
}

/**
 * Removes a node from the canvas.
 */
void VuoRendererComposition::removeNode(VuoRendererNode *rn)
{
	rn->updateGeometry();
	removeItem(rn);

	getBase()->removeNode(rn->getBase());
}

/**
 * Removes a cable from the canvas.
 */
void VuoRendererComposition::removeCable(VuoRendererCable *rc)
{
	rc->updateGeometry();
	rc->removeFromScene();

	getBase()->removeCable(rc->getBase());
}

/**
 * Removes a published input cable from the canvas and underlying composition.
 */
void VuoRendererComposition::removePublishedInputCable(VuoRendererCable *rc)
{
	rc->updateGeometry();
	rc->removeFromScene();

	getBase()->removePublishedInputCable(rc->getBase());
}

/**
 * Removes a published output cable from the canvas and underlying composition.
 */
void VuoRendererComposition::removePublishedOutputCable(VuoRendererCable *rc)
{
	rc->updateGeometry();
	rc->removeFromScene();

	getBase()->removePublishedOutputCable(rc->getBase());
}

/**
 * Creates and connects a "Make List" drawer to each of the provided node's list input ports.
 */
void VuoRendererComposition::createAndConnectDrawersToListInputPorts(VuoRendererNode *node, VuoCompiler *compiler)
{
	foreach (VuoPort *port, node->getBase()->getInputPorts())
	{
		VuoCompilerInputEventPort *inputEventPort = dynamic_cast<VuoCompilerInputEventPort *>(port->getCompiler());
		if (inputEventPort && VuoCompilerMakeListNodeClass::isListType(inputEventPort->getDataType()))
		{
			VuoRendererCable *cable = NULL;
			VuoRendererNode *makeListNode = createAndConnectMakeListNode(node->getBase(), port, compiler, cable);
			addNode(makeListNode->getBase());
			addCable(cable->getBase());
		}
	}
}

/**
 * Creates a "Make List" node, and creates a cable from the "Make List" node to the given input port.
 *
 * @param toNode The node that contains @c toPort.
 * @param toPort The input port. Assumed to be a data-and-event input port carrying list data.
 * @param compiler A compiler used to get the "Make List" node class.
 * @param[out] rendererCable The created cable.
 * @return The created "Make List" node.
 */
VuoRendererNode * VuoRendererComposition::createAndConnectMakeListNode(VuoNode *toNode, VuoPort *toPort, VuoCompiler *compiler,
																	   VuoRendererCable *&rendererCable)
{
	VuoRendererNode *makeListRendererNode = NULL;
	rendererCable = NULL;

	VuoCompilerInputEventPort *inputEventPort = static_cast<VuoCompilerInputEventPort *>(toPort->getCompiler());
	VuoCompilerType *type = inputEventPort->getDataType();

	vector<string> itemInitialValues;
	if (inputEventPort->getData())
	{
		string listInitialValue = inputEventPort->getData()->getInitialValue();
		json_object *js = json_tokener_parse(listInitialValue.c_str());
		if (json_object_get_type(js) == json_type_array)
		{
			int itemCount = json_object_array_length(js);
			for (int i = 0; i < itemCount; ++i)
			{
				json_object *itemObject = json_object_array_get_idx(js, i);
				string itemString = json_object_to_json_string_ext(itemObject, JSON_C_TO_STRING_PLAIN);
				itemInitialValues.push_back(itemString);
			}
		}
		json_object_put(js);
	}

	unsigned long itemCount = (itemInitialValues.empty() ? 2 : itemInitialValues.size());
	string nodeClassName = VuoCompilerMakeListNodeClass::getNodeClassName(itemCount, type);
	VuoCompilerNodeClass *makeListNodeClass = compiler->getNodeClass(nodeClassName);

	VuoNode *makeListNode = makeListNodeClass->newNode();
	makeListRendererNode = createRendererNode(makeListNode);

	vector<VuoPort *> itemPorts = makeListNode->getInputPorts();
	for (size_t i = 0; i < itemInitialValues.size(); ++i)
	{
		int portIndex = i + VuoNodeClass::unreservedInputPortStartIndex;
		VuoRendererPort *itemPort = itemPorts[portIndex]->getRenderer();
		itemPort->setConstant(itemInitialValues[i]);
	}

	VuoCompilerPort *fromCompilerPort = static_cast<VuoCompilerPort *>(makeListNode->getOutputPorts().back()->getCompiler());
	VuoCompilerPort *toCompilerPort = static_cast<VuoCompilerPort *>(toPort->getCompiler());
	VuoCompilerCable *compilerCable = new VuoCompilerCable(makeListNode->getCompiler(), fromCompilerPort,
														   toNode->getCompiler(), toCompilerPort);
	rendererCable = new VuoRendererCable(compilerCable->getBase());

	return makeListRendererNode;
}

/**
 * Creates a renderer detail for the pre-existing @c publishedPort, on the assumption that
 * the published port provided already exists in the base composition and has an associated compiler detail.
 */
VuoRendererPublishedPort * VuoRendererComposition::createRendererForPublishedPortInComposition(VuoPublishedPort *publishedPort)
{
	if (! publishedPort->hasCompiler())
		return NULL;

	return new VuoRendererPublishedPort(publishedPort);
}

/**
 * Adds an existing VuoPublishedPort as one of this composition's published ports.
 */
void VuoRendererComposition::addPublishedPort(VuoPublishedPort *publishedPort, bool isInput)
{
	string name = publishedPort->getName();
	if (isInput)
	{
		VuoPublishedPort *existingPort = getBase()->getPublishedInputPortWithName(name);
		if (! existingPort)
		{
			int index = getBase()->getPublishedInputPorts().size();
			getBase()->addPublishedInputPort(publishedPort, index);
			updatePublishedInputNode();
		}
		else if (publishedPort != existingPort)
			fprintf(stderr, "Error: Unhandled published port name conflict\n");
	}
	else // if (! isInput)
	{
		VuoPublishedPort *existingPort = getBase()->getPublishedOutputPortWithName(name);
		if (! existingPort)
		{
			int index = getBase()->getPublishedOutputPorts().size();
			getBase()->addPublishedOutputPort(publishedPort, index);
			updatePublishedOutputNode();
		}
		else if (publishedPort != existingPort)
			fprintf(stderr, "Error: Unhandled published port name conflict\n");
	}
}

/**
 * Removes a published input or output VuoRendererPublishedPort from the list
 * of published ports associated with this composition.
 *
 * @return The index within the list of published input port output ports at which the port was located, or -1 if not located.
 */
int VuoRendererComposition::removePublishedPort(VuoPublishedPort *publishedPort, bool isInput)
{
	if (isInput)
	{
		int index = getBase()->getIndexOfPublishedPort(publishedPort, isInput);
		if (index != -1)
		{
			getBase()->removePublishedInputPort(index);
			updatePublishedInputNode();
		}
		return index;
	}
	else
	{
		int index = getBase()->getIndexOfPublishedPort(publishedPort, isInput);
		if (index != -1)
		{
			getBase()->removePublishedOutputPort(index);
			updatePublishedOutputNode();
		}
		return index;
	}
}

/**
 * Sets the name of the provided @c publishedPort to @c name; updates the composition's
 * published pseudo-node and connected published cables accordingly.
 */
void VuoRendererComposition::setPublishedPortName(VuoRendererPublishedPort *publishedPort, string name)
{
	bool isInput = publishedPort->getBase()->getInput();
	publishedPort->setName(getUniquePublishedPortName(name, isInput));
	isInput? updatePublishedInputNode() : updatePublishedOutputNode();
}

/**
 * Updates the composition's published input node so that it remains consistent with the composition's
 * list of published input ports.
 * @todo: Incorporate type, not just name.
 */
void VuoRendererComposition::updatePublishedInputNode()
{
	// Derive the new published input node class from the composition's list of published input ports.
	vector<VuoPublishedPort *> publishedInputPorts = getBase()->getPublishedInputPorts();
	vector<string> publishedInputNodeOutputPortNames;
	foreach (VuoPublishedPort * publishedPort, publishedInputPorts)
		publishedInputNodeOutputPortNames.push_back(publishedPort->getName());

	VuoNodeClass *dummyVuoInNodeClass = new VuoNodeClass(VuoNodeClass::publishedInputNodeClassName, vector<string>(), publishedInputNodeOutputPortNames);
	VuoNodeClass *newVuoInNodeClass = VuoCompilerPublishedInputNodeClass::newNodeClass(dummyVuoInNodeClass);

	// Create the new published input node.
	VuoNode *newVuoInNode = newVuoInNodeClass->getCompiler()->newNode(VuoNodeClass::publishedInputNodeIdentifier, 0, 0);

	// Update the composition to reflect the newly created published input node.
	getBase()->getCompiler()->setPublishedInputNode(newVuoInNode);

	// Update the composition's published ports to reflect the relevant pseudo-ports of the newly created published input node
	// as their associated pseudo-ports.
	foreach (VuoPublishedPort *publishedPort, publishedInputPorts)
	{
		VuoCompilerPublishedInputPort *compilerPublishedInputPort = ((VuoCompilerPublishedInputPort *)(publishedPort->getCompiler()));

		vector<VuoCable *> publishedInputCables = compilerPublishedInputPort->getVuoPseudoPort()?
					compilerPublishedInputPort->getVuoPseudoPort()->getConnectedCables(true) :
					vector<VuoCable *>();

		VuoPort *newFromPort = newVuoInNode->getOutputPortWithName(publishedPort->getName());
		VuoCompilerTriggerPort *newFromTrigger = static_cast<VuoCompilerTriggerPort *>(newFromPort->getCompiler());
		compilerPublishedInputPort->setTriggerPort(newFromTrigger);

		// Update the port's connected published cables to reflect the newly created published input node as their 'From' node.
		foreach (VuoCable *cable, publishedInputCables)
			cable->setFrom(newVuoInNode, newFromPort);

		createRendererForPublishedPortInComposition(publishedPort);
	}
}

/**
 * Updates the composition's published output node so that it remains consistent with the composition's
 * list of published output ports.
 */
void VuoRendererComposition::updatePublishedOutputNode()
{
	// Derive the new published output node class from the composition's list of published output ports.
	vector<VuoPublishedPort *> publishedOutputPorts = getBase()->getPublishedOutputPorts();
	vector<string> publishedOutputNodeInputPortNames;
	foreach (VuoPublishedPort * publishedPort, publishedOutputPorts)
		publishedOutputNodeInputPortNames.push_back(publishedPort->getName());

	VuoNodeClass *newVuoOutNodeClass = new VuoNodeClass(VuoNodeClass::publishedOutputNodeClassName, publishedOutputNodeInputPortNames, vector<string>());

	// Create the new published output node.
	VuoNode *newVuoOutNode = newVuoOutNodeClass->newNode(VuoNodeClass::publishedOutputNodeIdentifier, 0, 0);

	// Update the composition to reflect the newly created published output node.
	getBase()->getCompiler()->setPublishedOutputNode(newVuoOutNode);

	// Update the composition's published ports to reflect the relevant pseudo-ports of the newly created published output node
	// as their associated pseudo-ports.
	foreach (VuoPublishedPort *publishedPort, publishedOutputPorts)
	{
		VuoCompilerPublishedOutputPort *compilerPublishedOutputPort = ((VuoCompilerPublishedOutputPort *)(publishedPort->getCompiler()));

		vector<VuoCable *> publishedOutputCables = compilerPublishedOutputPort->getVuoPseudoPort()?
					compilerPublishedOutputPort->getVuoPseudoPort()->getConnectedCables(true) :
					vector<VuoCable *>();

		VuoPort *newToPort = newVuoOutNode->getInputPortWithName(publishedPort->getName());
		compilerPublishedOutputPort->setVuoPseudoPort(newToPort);

		// Update the port's connected published cables to reflect the newly created published output node as their 'To' node.
		foreach (VuoCable *cable, publishedOutputCables)
			cable->setTo(newVuoOutNode, newToPort);

		createRendererForPublishedPortInComposition(publishedPort);
	}
}

/**
 * Returns a string derived from the input @c baseName that is guaranteed
 * to be unique either among the published input port names or among the published
 * output port names for this composition, as specified by @c isInput.
 */
string VuoRendererComposition::getUniquePublishedPortName(string baseName, bool isInput)
{
	string uniquePortName = baseName;
	string uniquePortNamePrefix = uniquePortName;
	int portNameInstanceNum = 1;
	while (isPublishedPortNameTaken(uniquePortName, isInput))
	{
		ostringstream oss;
		oss << ++portNameInstanceNum;
		uniquePortName = uniquePortNamePrefix + "_" + oss.str();
	}

	return uniquePortName;
}

/**
 * Returns a boolean indicating whether the input @c name is already taken
 * either by a published input port or by a published output port associated with this
 * composition, as specified by @c isInput.
 */
bool VuoRendererComposition::isPublishedPortNameTaken(string name, bool isInput)
{
	if (name == "refresh" || name == "done")
		return true;

	VuoPublishedPort *publishedPort = (isInput ?
										   getBase()->getPublishedInputPortWithName(name) :
										   getBase()->getPublishedOutputPortWithName(name));
	return (publishedPort != NULL);
}

/**
 * Once all nodes and cables have been added to the scene, call this to convert each freestanding typecast node into a mini-node attached to its destination node.
 * Returns a vector of pointers to the newly collapsed nodes.
 */
vector<VuoRendererNode *> VuoRendererComposition::collapseTypecastNodes(void)
{
	vector<VuoRendererNode *> typecastsCollapsed;

	foreach (VuoNode *node, getBase()->getNodes())
	{
		if (node->isTypecastNode())
		{
			VuoRendererTypecastPort *collapsedPort = collapseTypecastNode(node->getRenderer());
			if (collapsedPort)
				typecastsCollapsed.push_back(node->getRenderer());
		}
	}

	return typecastsCollapsed;
}

/**
 * Convert a freestanding typecast node into a mini-node attached to its destination node;
 * returns a pointer to the newly collapsed node.
 */
VuoRendererTypecastPort * VuoRendererComposition::collapseTypecastNode(VuoRendererNode *rn)
{
	VuoPort * typecastInPort = rn->getBase()->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex];
	VuoPort * typecastOutPort = rn->getBase()->getOutputPorts()[VuoNodeClass::unreservedOutputPortStartIndex];

	// Don't try to collapse nodes that don't qualify as typecasts.
	if (!rn->getBase()->isTypecastNode())
		return NULL;

	// Don't try to re-collapse typecasts that are already collapsed.
	VuoRendererPort *typecastParent = typecastInPort->getRenderer()->getTypecastParentPort();
	if (typecastParent)
		return NULL;

	vector<VuoCable *> outCables = typecastOutPort->getConnectedCables(true);
	vector<VuoCable *> inCables = typecastInPort->getConnectedCables(true);

	// Don't try to collapse typecast nodes with incoming cables to the "refresh" port.
	if ( ! rn->getBase()->getRefreshPort()->getConnectedCables(true).empty() )
		return NULL;

	// Don't try to collapse typecast nodes with outgoing cables from the "done" port.
	if ( ! rn->getBase()->getDonePort()->getConnectedCables(true).empty() )
		return NULL;

	// Don't try to collapse typecast nodes outputting to multiple nodes, or without any output cables.
	if (outCables.size() != 1)
		return NULL;

	// Don't try to collapse typecast nodes without any incoming data+event cables (including published ones).
	VuoCable *incomingDataCable = NULL;
	for (vector<VuoCable *>::iterator i = inCables.begin(); !incomingDataCable && (i != inCables.end()); ++i)
	{
		if ((*i)->hasRenderer() && (*i)->getRenderer()->effectivelyCarriesData())
			incomingDataCable = *i;
	}
	if (! incomingDataCable)
		return NULL;

	// Don't try to collapse typecast nodes that have published output ports.
	bool hasPublishedOutputPort = (! getBase()->getPublishedOutputPortsConnectedToNode(rn->getBase()).empty());
	if (hasPublishedOutputPort)
		return NULL;

	VuoNode *fromNode = incomingDataCable->getFromNode();
	VuoPort *fromPort = incomingDataCable->getFromPort();

	VuoCable * outCable = *(outCables.begin());
	VuoNode * toNode = outCable->getToNode();
	VuoPort * toPort = outCable->getToPort();

	// Don't try to collapse typecast nodes with input or output cables that are not currently connected at both ends.
	if (! (fromNode && fromPort && toNode && toPort))
		return NULL;

	// Don't try to collapse typecast nodes with attached input drawers.
	if (rn->getAttachedInputDrawers().size() > 0)
		return NULL;

	// Don't try to collapse typecast nodes outputting to other typecasts.
	if (toNode->isTypecastNode())
		return NULL;

	// Don't try to collapse typecast nodes outputting to ports with multiple input cables.
	if (toPort->getConnectedCables(false).size() > 1)
		return NULL;

	// Don't try to collapse typecast nodes outputting to published ports.
	if (!getBase()->getPublishedInputPortsConnectedToPort(toPort).empty())
		return NULL;

	// Hide the typecast node.
	VuoRendererNode * toRN = toNode->getRenderer();
	rn->updateGeometry();
	rn->setProxyNode(toRN);

	// Replace the target node's input port with a new typecast port.
	VuoRendererPort * oldToRP = toPort->getRenderer();
	VuoRendererTypecastPort *tp = new VuoRendererTypecastPort(rn,
															  oldToRP,
															  signaler);

	foreach (VuoCable *cable, inCables)
	{
		if (cable->hasRenderer())
			cable->getRenderer()->updateGeometry();
	}

	foreach (VuoCable *cable, outCables)
	{
		if (cable->hasRenderer())
			cable->getRenderer()->updateGeometry();
	}

	tp->updateGeometry();
	toRN->replaceInputPort(oldToRP, tp);
	toRN->setCacheModeForNodeAndPorts(getCurrentDefaultCacheMode());
	typecastInPort->getRenderer()->setTypecastParentPort(tp);
	typecastInPort->getRenderer()->setParentItem(toRN);

	// Notify the base port of the change in renderer port, to reverse
	// the change made within the VuoRendererPort constructor on behalf of any
	// renderer port previously initialized for this base port.
	tp->getBase()->setRenderer(tp);

	toRN->layoutConnectedInputDrawersAtAndAbovePort(tp);

	return tp;
}

/**
 * Convert the collapsed typecast mini-node associated with the
 * input @c typecastNode back into freestanding form.
 */
void VuoRendererComposition::uncollapseTypecastNode(VuoRendererNode *typecastNode)
{
	VuoPort * typecastInPort = typecastNode->getBase()->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex];
	VuoRendererPort *typecastParent = typecastInPort->getRenderer()->getTypecastParentPort();

	if (typecastParent)
		uncollapseTypecastNode((VuoRendererTypecastPort *)typecastParent);
}

/**
 * Convert a collapsed typecast mini-node back into a freestanding node.
 */
VuoRendererNode * VuoRendererComposition::uncollapseTypecastNode(VuoRendererTypecastPort *typecast)
{
	VuoRendererNode *uncollapsedNode = typecast->getUncollapsedTypecastNode();
	VuoPort *typecastInPort = uncollapsedNode->getBase()->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex];
	VuoPort *typecastOutPort = uncollapsedNode->getBase()->getOutputPorts()[VuoNodeClass::unreservedOutputPortStartIndex];
	vector<VuoCable *> outCables = typecastOutPort->getConnectedCables(false);
	VuoCable * outCable = *(outCables.begin());
	VuoRendererPort *uncollapsedToRP = typecast->getReplacedPort();
	VuoRendererNode *toRN = outCable->getToNode()->getRenderer();

	foreach (VuoCable *cable, typecastInPort->getConnectedCables(true))
	{
		if (cable->hasRenderer())
			cable->getRenderer()->updateGeometry();
	}

	foreach (VuoCable *cable, typecastOutPort->getConnectedCables(true))
	{
		if (cable->hasRenderer())
			cable->getRenderer()->updateGeometry();
	}

	typecast->updateGeometry();
	uncollapsedNode->updateGeometry();

	typecastInPort->getRenderer()->setParentItem(uncollapsedNode);
	uncollapsedNode->addInputPort(typecastInPort->getRenderer());

	typecastInPort->getRenderer()->setTypecastParentPort(NULL);
	uncollapsedNode->setProxyNode(NULL);

	toRN->updateGeometry();
	toRN->replaceInputPort(typecast, uncollapsedToRP);
	toRN->setCacheModeForNodeAndPorts(getCurrentDefaultCacheMode());

	// Notify the base port of the change in renderer port, to reverse
	// the change made within the VuoRendererPort constructor on behalf of any
	// renderer port previously initialized for this base port.
	uncollapsedToRP->getBase()->setRenderer(uncollapsedToRP);

	toRN->layoutConnectedInputDrawersAtAndAbovePort(uncollapsedToRP);

	return uncollapsedNode;
}

/**
 * Removes connection eligibility highlighting from all ports in the scene.
 */
void VuoRendererComposition::clearInternalPortEligibilityHighlighting()
{
	foreach (VuoNode *node, getBase()->getNodes())
	{
		vector<VuoPort *> inputPorts = node->getInputPorts();
		for(vector<VuoPort *>::iterator inputPort = inputPorts.begin(); inputPort != inputPorts.end(); ++inputPort)
		{
			QGraphicsItem::CacheMode normalCacheMode = (*inputPort)->getRenderer()->cacheMode();
			(*inputPort)->getRenderer()->setCacheMode(QGraphicsItem::NoCache);

			(*inputPort)->getRenderer()->updateGeometry();
			(*inputPort)->getRenderer()->setEligibleForDirectConnection(false);
			(*inputPort)->getRenderer()->setEligibleForConnectionViaTypecast(false);

			(*inputPort)->getRenderer()->setCacheMode(normalCacheMode);

			VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>((*inputPort)->getRenderer());
			if (typecastPort)
			{
				QGraphicsItem::CacheMode normalCacheMode = typecastPort->getChildPort()->cacheMode();
				typecastPort->getChildPort()->setCacheMode(QGraphicsItem::NoCache);

				typecastPort->getChildPort()->updateGeometry();
				typecastPort->getChildPort()->setEligibleForDirectConnection(false);
				typecastPort->getChildPort()->setEligibleForConnectionViaTypecast(false);

				typecastPort->getChildPort()->setCacheMode(normalCacheMode);
			}
		}

		vector<VuoPort *> outputPorts = node->getOutputPorts();
		for(vector<VuoPort *>::iterator outputPort = outputPorts.begin(); outputPort != outputPorts.end(); ++outputPort)
		{
			QGraphicsItem::CacheMode normalCacheMode = (*outputPort)->getRenderer()->cacheMode();
			(*outputPort)->getRenderer()->setCacheMode(QGraphicsItem::NoCache);

			(*outputPort)->getRenderer()->updateGeometry();
			(*outputPort)->getRenderer()->setEligibleForDirectConnection(false);
			(*outputPort)->getRenderer()->setEligibleForConnectionViaTypecast(false);

			(*outputPort)->getRenderer()->setCacheMode(normalCacheMode);
		}
	}
}

/**
 * Prepares every component in the composition to be repainted.
 */
void VuoRendererComposition::repaintAllComponents()
{
	foreach (VuoNode *node, getBase()->getNodes())
	{
		node->getRenderer()->updateGeometry();

		foreach (VuoPort *port, node->getInputPorts())
			port->getRenderer()->updateGeometry();

		foreach (VuoPort *port, node->getOutputPorts())
			port->getRenderer()->updateGeometry();
	}

	foreach (VuoCable *cable, getBase()->getCables())
		cable->getRenderer()->updateGeometry();

	foreach (VuoCable *cable, getBase()->getPublishedInputCables())
		cable->getRenderer()->updateGeometry();

	foreach (VuoCable *cable, getBase()->getPublishedOutputCables())
		cable->getRenderer()->updateGeometry();
}

/**
 * Returns the boolean indicating whether recent activity (e.g., node executions,
 * event firings) by components within this composition should be reflected in
 * the rendering of the composition.
 */
bool VuoRendererComposition::getRenderActivity(void)
{
	return this->renderActivity;
}

/**
 * Sets the boolean indicating whether recent activity by components within
 * this composition should be reflected in the rendering of the composition;
 * if toggling from 'false' to 'true', resets the time of last activity
 * for each component.
 */
void VuoRendererComposition::setRenderActivity(bool render)
{
	if (this->renderActivity == render)
		return;

	this->renderActivity = render;

	if (render)
	{
		foreach (VuoNode *node, getBase()->getNodes())
		{
			node->getRenderer()->resetTimeLastExecuted();

			foreach (VuoPort *port, node->getInputPorts())
				port->getRenderer()->resetTimeLastEventFired();

			foreach (VuoPort *port, node->getOutputPorts())
				port->getRenderer()->resetTimeLastEventFired();
		}

		foreach (VuoCable *cable, getBase()->getCables())
			cable->getRenderer()->resetTimeLastEventPropagated();

		foreach (VuoCable *cable, getBase()->getPublishedInputCables())
			cable->getRenderer()->resetTimeLastEventPropagated();

		foreach (VuoCable *cable, getBase()->getPublishedOutputCables())
			cable->getRenderer()->resetTimeLastEventPropagated();
	}

	repaintAllComponents();
	updateComponentCaching();
}

/**
 * Suspends or resumes caching for each applicable graphics item within the
 * composition, as determined by the current default cache mode returned by
 * VuoRendererComposition::getCurrentDefaultCacheMode().
 */
void VuoRendererComposition::updateComponentCaching()
{
	QGraphicsItem::CacheMode currentDefaultCacheMode = getCurrentDefaultCacheMode();

	// Nodes and ports
	foreach (VuoNode *node, getBase()->getNodes())
	{
		VuoRendererNode *rn = node->getRenderer();
		if (rn)
			rn->setCacheModeForNodeAndPorts(currentDefaultCacheMode);
	}

	// Cables
	set<VuoCable *> internalCables = getBase()->getCables();
	set<VuoCable *> publishedInputCables = getBase()->getPublishedInputCables();
	set<VuoCable *> publishedOutputCables = getBase()->getPublishedOutputCables();

	set<VuoCable *> allCables;
	allCables.insert(internalCables.begin(), internalCables.end());
	allCables.insert(publishedInputCables.begin(), publishedInputCables.end());
	allCables.insert(publishedOutputCables.begin(), publishedOutputCables.end());

	foreach (VuoCable *cable, allCables)
	{
		VuoRendererCable *rc = cable->getRenderer();
		if (rc)
			rc->setCacheMode(currentDefaultCacheMode);
	}
}

/**
 * Returns the current default cache mode for components of this composition, dependent
 * on whether caching is enabled for this composition in general and on whether
 * running composition activity is currently being reflected in the composition rendering.
 */
QGraphicsItem::CacheMode VuoRendererComposition::getCurrentDefaultCacheMode()
{
	return ((cachingEnabled && !renderActivity)? QGraphicsItem::DeviceCoordinateCache : QGraphicsItem::NoCache);
}

/**
 * As a workaround for a bug in Qt 5.1.0-beta1 (https://b33p.net/kosada/node/4905),
 * this function must be called to create the NSAutoreleasePool for a QApplication.
 */
void VuoRendererComposition::createAutoreleasePool(void)
{
#ifdef MAC
	// [NSAutoreleasePool new];
	Class poolClass = (Class)objc_getClass("NSAutoreleasePool");
	SEL newSEL = sel_registerName("new");
	Method poolNewMethod = class_getClassMethod(poolClass, newSEL);
	IMP poolNew = method_getImplementation(poolNewMethod);
	poolNew((id)poolClass, method_getName(poolNewMethod));
#endif
}

/**
 * Returns a string representation of the composition (to save its current state).
 */
string VuoRendererComposition::takeSnapshot(void)
{
	return (getBase()->hasCompiler()? getBase()->getCompiler()->getGraphvizDeclaration() : NULL);
}

/**
 * Exports the composition as an OS X .app bundle.
 *
 * @param[in] savePath The path where the .app is to be saved.
 * @param[in] compiler The compiler to be used to generate the composition executable.
 * @param[out] errString The error message resulting from the export process, if any.
 * @return An @c appExportResult value detailing the outcome of the export attempt.
 */
VuoRendererComposition::appExportResult VuoRendererComposition::exportApp(const QString &savePath, VuoCompiler *compiler, string &errString)
{
	// Set up the directory structure for the app bundle in a temporary location.
	string tmpAppPath = createAppBundleDirectoryStructure();

	// Generate and bundle the composition executable.
	string dir, file, ext;
	VuoFileUtilities::splitPath(savePath.toUtf8().constData(), dir, file, ext);
	string buildErrString = "";
	if (!bundleExecutable(compiler, tmpAppPath + "/Contents/MacOS/" + file, buildErrString))
	{
		errString = buildErrString;
		return exportBuildFailure;
	}

	// Generate and bundle the Info.plist.
	string plist = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"><plist version=\"1.0\"><dict><key>NSHighResolutionCapable</key><true/></dict></plist>";
	VuoFileUtilities::writeStringToFile(plist, tmpAppPath + "/Contents/Info.plist");

	// Bundle the essential components of Vuo.framework.
	string sourceVuoFrameworkPath = VuoFileUtilities::getVuoFrameworkPath();
	string targetVuoFrameworkPath = tmpAppPath + "/Contents/Frameworks/Vuo.framework/Versions/" + VUO_VERSION_STRING;
	bundleVuoSubframeworks(sourceVuoFrameworkPath, targetVuoFrameworkPath);
	bundleVuoFrameworkFolder(sourceVuoFrameworkPath + "/Modules", targetVuoFrameworkPath + "/Modules", "dylib");
	bundleVuoFrameworkFolder(sourceVuoFrameworkPath + "/Licenses", targetVuoFrameworkPath + "/Licenses");

	// Move the generated app bundle to the desired save path.
	bool saveSucceeded = (! rename(tmpAppPath.c_str(), savePath.toUtf8().constData()));
	if (!saveSucceeded)
		return exportSaveFailure;

	return exportSuccess;
}

/**
 * Sets up the directory structure for an .app bundle in a temporary location.
 *
 * Helper function for VuoRendererComposition::exportApp(const QString &savePath).
 * @return The path of the temporary .app bundle.
 */
string VuoRendererComposition::createAppBundleDirectoryStructure()
{
	string appPath = VuoFileUtilities::makeTmpDir("VuoExportedApp");

	string contentsPath = appPath + "/Contents";
	mkdir(contentsPath.c_str(), 0755);

	string macOSPath = contentsPath + "/MacOS";
	mkdir(macOSPath.c_str(), 0755);

	string resourcesPath = contentsPath + "/Resources";
	mkdir(resourcesPath.c_str(), 0755);

	string frameworksPath = contentsPath + "/Frameworks";
	mkdir(frameworksPath.c_str(), 0755);

	string vuoFrameworkPath = frameworksPath + "/Vuo.framework";
	mkdir(vuoFrameworkPath.c_str(), 0755);

	string vuoFrameworksPathVersions = vuoFrameworkPath + "/Versions";
	mkdir(vuoFrameworksPathVersions.c_str(), 0755);

	string vuoFrameworksPathVersionsCurrent = vuoFrameworksPathVersions + "/" + VUO_VERSION_STRING;
	mkdir(vuoFrameworksPathVersionsCurrent.c_str(), 0755);

	string vuoFrameworksPathVersionsCurrentFrameworks = vuoFrameworksPathVersionsCurrent + "/Frameworks";
	mkdir(vuoFrameworksPathVersionsCurrentFrameworks.c_str(), 0755);

	string vuoFrameworksPathVersionsCurrentModules = vuoFrameworksPathVersionsCurrent + "/Modules";
	mkdir(vuoFrameworksPathVersionsCurrentModules.c_str(), 0755);

	string vuoFrameworksPathVersionsCurrentLicenses = vuoFrameworksPathVersionsCurrent + "/Licenses";
	mkdir(vuoFrameworksPathVersionsCurrentLicenses.c_str(), 0755);

	return appPath;
}

/**
 * Compiles and links this composition to create an executable.
 *
 * Helper function for VuoRendererComposition::exportApp(const QString &savePath).
 * @param[in] targetExecutablePath The path where the executable is to be saved.
 * @param[out] errString The error message resulting from the build process, if any.
 * @return @c true on success, @c false on failure.
 */
bool VuoRendererComposition::bundleExecutable(VuoCompiler *compiler, string targetExecutablePath, string &errString)
{
	// Generate the executable.
	try
	{
		VuoCompilerComposition *compiledCompositionToExport = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(takeSnapshot(), compiler);
		string pathOfCompiledCompositionToExport = VuoFileUtilities::makeTmpFile(compiledCompositionToExport->getBase()->getName(), "bc");

		compiler->compileComposition(compiledCompositionToExport, pathOfCompiledCompositionToExport);

		string rPath = "@loader_path/../Frameworks";
		compiler->linkCompositionToCreateExecutable(pathOfCompiledCompositionToExport, targetExecutablePath, true, rPath);
		remove(pathOfCompiledCompositionToExport.c_str());
	}

	catch (const runtime_error &e)
	{
		errString = e.what();
		return false;
	}

	return true;
}

/**
 * Copies the contents of the directory from the Vuo framework located at @c sourceVuoFrameworkPath
 * to the directory within the Vuo framework located at @c targetVuoFrameworkPath.
 * Assumes that these directories exist already.
 *
 * If @c onlyCopyExtension is emptystring, all files are copied.
 * Otherwise, only files with the specified extension are copied.
 *
 * Helper function for VuoRendererComposition::exportApp(const QString &savePath).
 */
void VuoRendererComposition::bundleVuoFrameworkFolder(string sourceVuoFrameworkPath, string targetVuoFrameworkPath, string onlyCopyExtension)
{
	QDir sourceVuoFrameworkDir(sourceVuoFrameworkPath.c_str());
	QDir targetVuoFrameworkDir(targetVuoFrameworkPath.c_str());

	if (sourceVuoFrameworkDir.exists())
	{
		QStringList vuoFrameworkModulesList(sourceVuoFrameworkDir.entryList(QDir::Files|QDir::Readable));

		// Vuo.framework/Modules/<topLevelFiles>
		foreach (QString vuoFrameworkModuleName, vuoFrameworkModulesList)
		{
			string dir, file, extension;
			VuoFileUtilities::splitPath(vuoFrameworkModuleName.toUtf8().constData(), dir, file, extension);

			if (!onlyCopyExtension.length() || extension == onlyCopyExtension)
				QFile::copy(sourceVuoFrameworkDir.filePath(vuoFrameworkModuleName),
							targetVuoFrameworkDir.filePath(vuoFrameworkModuleName));
		}
	}
}

/**
 * Copies the essential contents of the Vuo subframeworks from the Vuo framework located at
 * @c sourceVuoFrameworkPath to the Vuo framework located at @c targetVuoFrameworkPath.
 * Assumes that the "Frameworks" directory exists within the source and target directories already.
 *
 * Helper function for VuoRendererComposition::exportApp(const QString &savePath).
 */
void VuoRendererComposition::bundleVuoSubframeworks(string sourceVuoFrameworkPath, string targetVuoFrameworkPath)
{
	QDir sourceVuoSubframeworksPath((sourceVuoFrameworkPath + "/Frameworks").c_str());
	QDir targetVuoSubframeworksPath((targetVuoFrameworkPath + "/Frameworks").c_str());

	set<string> subframeworksToExclude;
	subframeworksToExclude.insert("CRuntime.framework");
	subframeworksToExclude.insert("VuoRuntime.framework");
	subframeworksToExclude.insert("clang.framework");
	subframeworksToExclude.insert("llvm.framework");
	subframeworksToExclude.insert("zmq.framework");

	if (sourceVuoSubframeworksPath.exists())
	{
		QStringList subframeworkDirList(sourceVuoSubframeworksPath.entryList(QDir::Dirs|QDir::Readable|QDir::NoDotAndDotDot));
		foreach (QString subframeworkDirName, subframeworkDirList)
		{
			if (subframeworksToExclude.find(subframeworkDirName.toUtf8().constData()) == subframeworksToExclude.end())
			{
				// Vuo.framework/Frameworks/<x>.framework
				QDir targetVuoSubframeworkPath(targetVuoSubframeworksPath.filePath(subframeworkDirName));
				mkdir(targetVuoSubframeworkPath.absolutePath().toUtf8().constData(), 0755);

				QDir sourceVuoSubframeworkPath(sourceVuoSubframeworksPath.filePath(subframeworkDirName));
				QStringList subframeworkDirContentsList(sourceVuoSubframeworkPath.entryList(QDir::Files|QDir::Readable));

				// Vuo.framework/Frameworks/<x>.framework/<topLevelFiles>
				foreach (QString subframeworkTopLevelFile, subframeworkDirContentsList)
				{
					QFile::copy(sourceVuoSubframeworkPath.filePath(subframeworkTopLevelFile),
								targetVuoSubframeworkPath.filePath(subframeworkTopLevelFile));
				}
			}
		}
	}
}
