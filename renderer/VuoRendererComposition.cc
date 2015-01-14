/**
 * @file
 * VuoRendererComposition implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>

#include "VuoRendererComposition.hh"

#include "VuoCompilerCable.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerGraphvizParser.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerMakeListNodeClass.hh"
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

#ifdef MAC
#include <QtMacExtras/QMacFunctions>
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
	{
		rn = new VuoRendererNode(baseNode, signaler);

		// Performance optimizations
		// Caching is currently disabled for VuoRendererMakeListNodes; see
		// https://b33p.net/kosada/node/6286 and https://b33p.net/kosada/node/6064 .
		if (cachingEnabled && !renderActivity)
			rn->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	}

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

	if (rn->getBase()->hasCompiler())
	{
		string uniqueNodeName = rn->getBase()->getCompiler()->getGraphvizIdentifier();
		if (nodeNameTaken[uniqueNodeName] != NULL && nodeNameTaken[uniqueNodeName] != rn)
		{
			uniqueNodeName = rn->getBase()->getCompiler()->getGraphvizIdentifierPrefix();
			string uniqueNodeNamePrefix = uniqueNodeName;
			int nodeNameInstanceNum = 1;
			while (nodeNameTaken[uniqueNodeName])
			{
				ostringstream oss;
				oss << ++nodeNameInstanceNum;
				uniqueNodeName = uniqueNodeNamePrefix + oss.str();
			}
			rn->getBase()->getCompiler()->setGraphvizIdentifier(uniqueNodeName);
		}
		nodeNameTaken[uniqueNodeName] = rn;
	}

	rn->layoutConnectedInputDrawers();
	addItem(rn);
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
 * Creates a renderer detail for the pre-existing @c publishedPort, on the assumption that
 * the published port provided already exists in the base composition and has an associated compiler detail.
 */
VuoRendererPublishedPort * VuoRendererComposition::createRendererForPublishedPortInComposition(VuoPublishedPort *publishedPort)
{
	if (! publishedPort->hasCompiler())
		return NULL;

	VuoPort *vuoPort = publishedPort->getCompiler()->getVuoPseudoPort();
	VuoRendererPort *RP = new VuoRendererPort(vuoPort, signaler, true, false, false, false);
	VuoRendererPublishedPort *RPP = new VuoRendererPublishedPort(publishedPort);
	RP->setProxyPublishedSidebarPort(RPP);

	return RPP;
}

/**
 * Publishes this composition's internal @c port under the provided @c name, if possible;
 * returns a pointer to the VuoRendererPublishedPort aliased to the internal port.
 * If the requested @c name is already taken by an existing external published port and
 * @c attemptMerge is true, the existing external port will accommodate the new internal port
 * provided that their types are compatible and the new port would not displace any previously
 * connected port. Otherwise, the newly published internal port will be published under a unique name
 * derived from the requested name.
 */
VuoRendererPublishedPort * VuoRendererComposition::publishPort(VuoPort *port, string name, bool attemptMerge)
{
	string publishedPortName = ((! name.empty())? name : port->getClass()->getName());
	bool isInput = port->getRenderer()->getInput();
	VuoPublishedPort *publishedPort = NULL;

	// If merging is enabled:
	// Check whether this composition has a pre-existing externally visible published port
	// that has the requested name and that can accommodate the newly published internal port.
	// If so, add this internal port as a connected port for the existing alias.
	bool performedMerge = false;
	if (attemptMerge)
	{
		publishedPort = (isInput ?
							 getBase()->getPublishedInputPortWithName(publishedPortName) :
							 getBase()->getPublishedOutputPortWithName(publishedPortName));

		if (publishedPort && publishedPort->getRenderer()->canAccommodateInternalPort(port->getRenderer()))
		{
			publishedPort->addConnectedPort(port);
			performedMerge = true;
		}
	}


	// Otherwise, create a new externally visible published port with a unique name derived from
	// the specified name, containing the current port as its lone connected internal port.
	if (! performedMerge)
	{
		set<VuoCompilerPort *> connectedPorts;
		connectedPorts.insert((VuoCompilerPort *)(port->getCompiler()));

		publishedPort = (isInput?
								 (new VuoCompilerPublishedInputPort(getUniquePublishedPortName(publishedPortName, true), connectedPorts, NULL))->getBase() :
								 (new VuoCompilerPublishedOutputPort(getUniquePublishedPortName(publishedPortName, false), (VuoCompilerPort *)(port->getCompiler()), NULL))->getBase()
							);
	}

	registerExternalPublishedPort(publishedPort, isInput);

	VuoRendererPublishedPort *rendererPublishedPort = (publishedPort->hasRenderer()?
															  publishedPort->getRenderer() :
															  createRendererForPublishedPortInComposition(publishedPort));

	VuoCable *existingPublishedCable = port->getCableConnecting(publishedPort->getCompiler()->getVuoPseudoPort());

	if (! existingPublishedCable)
	{
		VuoCable *publishedCable = createPublishedCable(publishedPort->getCompiler()->getVuoPseudoPort(), port);
		isInput? addPublishedInputCable(publishedCable) : addPublishedOutputCable(publishedCable);
	}

	return rendererPublishedPort;
}

/**
 * Creates a published cable connecting published pseudo-port @c vuoPort with
 * internal port @c internalPort (in whichever direction appropriate).
 */
VuoCable * VuoRendererComposition::createPublishedCable(VuoPort *vuoPseudoPort, VuoPort *internalPort)
{
	VuoCable *publishedCable = NULL;
	bool creatingPublishedInputCable = internalPort->getRenderer()->getInput();

	if (creatingPublishedInputCable)
	{
		// If creating a published input cable, it will need to have an associated VuoCompilerCable.
		VuoPort *fromPort = vuoPseudoPort;
		VuoNode *fromNode = getBase()->getCompiler()->getPublishedInputNode();
		VuoPort *toPort = internalPort;
		VuoNode *toNode = internalPort->getRenderer()->getUnderlyingParentNode()->getBase();

		publishedCable = (new VuoCompilerCable(fromNode->getCompiler(),
											  (VuoCompilerPort *)(fromPort->getCompiler()),
											  toNode->getCompiler(),
											  (VuoCompilerPort *)(toPort->getCompiler())))->getBase();
	}

	else
	{
		// If creating a published output cable, it will need to have an associated VuoCompilerCable
		// even though we don't currently construct a VuoCompilerNode for the published output node.
		VuoPort *fromPort = internalPort;
		VuoNode *fromNode = internalPort->getRenderer()->getUnderlyingParentNode()->getBase();
		VuoPort *toPort = vuoPseudoPort;
		VuoNode *toNode = getBase()->getCompiler()->getPublishedOutputNode();

		publishedCable = (new VuoCompilerCable(fromNode->getCompiler(),
											  (VuoCompilerPort *)(fromPort->getCompiler()),
											  NULL,
											   NULL))->getBase();
		publishedCable->setTo(toNode, toPort);

	}

	return publishedCable;
}


/**
 * Registers a previously existing VuoPublishedPort as one of this composition's
 * published ports.
 */
void VuoRendererComposition::registerExternalPublishedPort(VuoPublishedPort *publishedPort, bool isInput)
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
int VuoRendererComposition::unregisterExternalPublishedPort(VuoPublishedPort *publishedPort, bool isInput)
{
	if (isInput)
	{
		int index = getIndexOfPublishedPort(publishedPort, isInput);
		if (index != -1)
		{
			getBase()->removePublishedInputPort(index);
			updatePublishedInputNode();
		}
		return index;
	}
	else
	{
		int index = getIndexOfPublishedPort(publishedPort, isInput);
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
	publishedPort->getBase()->setName(getUniquePublishedPortName(name, isInput));
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
	VuoNodeClass *newVuoInNodeClass = VuoCompilerNodeClass::createPublishedInputNodeClass(dummyVuoInNodeClass->getOutputPortClasses());

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
		if ((*i)->hasCompiler() && (*i)->getCompiler()->carriesData())
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
	if (getBase()->getPublishedInputPortConnectedToPort(toPort))
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
	tp->updateGeometry();

	toRN->replaceInputPort(oldToRP, tp);
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

	typecast->updateGeometry();
	uncollapsedNode->updateGeometry();

	typecastInPort->getRenderer()->setParentItem(uncollapsedNode);
	uncollapsedNode->addInputPort(typecastInPort->getRenderer());

	typecastInPort->getRenderer()->setTypecastParentPort(NULL);
	uncollapsedNode->setProxyNode(NULL);

	toRN->updateGeometry();
	toRN->replaceInputPort(typecast, uncollapsedToRP);

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
			(*inputPort)->getRenderer()->updateGeometry();
			(*inputPort)->getRenderer()->setEligibleForConnection(false);

			VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>((*inputPort)->getRenderer());
			if (typecastPort)
			{
				typecastPort->getChildPort()->updateGeometry();
				typecastPort->getChildPort()->setEligibleForConnection(false);
			}
		}

		vector<VuoPort *> outputPorts = node->getOutputPorts();
		for(vector<VuoPort *>::iterator outputPort = outputPorts.begin(); outputPort != outputPorts.end(); ++outputPort)
		{
			(*outputPort)->getRenderer()->updateGeometry();
			(*outputPort)->getRenderer()->setEligibleForConnection(false);
		}
	}
}

/**
 * Returns the index of the published port in the list of published input or output ports.
 */
int VuoRendererComposition::getIndexOfPublishedPort(VuoPublishedPort *port, bool isInput)
{
	vector<VuoPublishedPort *> publishedPorts = (isInput ? getBase()->getPublishedInputPorts() : getBase()->getPublishedOutputPorts());
	vector<VuoPublishedPort *>::iterator foundPort = find(publishedPorts.begin(), publishedPorts.end(), port);
	if (foundPort != publishedPorts.end())
		return std::distance(publishedPorts.begin(), foundPort);
	else
		return -1;
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
