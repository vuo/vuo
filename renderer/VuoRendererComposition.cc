/**
 * @file
 * VuoRendererComposition implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>
#include "../node/vuo.file/VuoFileFormat.h"

#include "VuoRendererComposition.hh"

#include "VuoCompiler.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerDriver.hh"
#include "VuoCompilerGraphvizParser.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerMakeListNodeClass.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerType.hh"

#include "VuoRendererNode.hh"
#include "VuoRendererInputListDrawer.hh"
#include "VuoRendererReadOnlyDictionary.hh"
#include "VuoRendererKeyListForReadOnlyDictionary.hh"
#include "VuoRendererValueListForReadOnlyDictionary.hh"
#include "VuoRendererPort.hh"
#include "VuoRendererCable.hh"
#include "VuoRendererTypecastPort.hh"
#include "VuoRendererSignaler.hh"
#include "VuoRendererColors.hh"
#include "VuoRendererFonts.hh"

#include "VuoPort.hh"

#include "VuoHeap.h"

#include "VuoUrl.h"

#include <sys/stat.h>
#include <regex.h>

#ifdef MAC
#include <objc/runtime.h>
#endif

int VuoRendererComposition::gridLineOpacity = 0;
const int VuoRendererComposition::minorGridLineSpacing = VuoRendererPort::portSpacing;
const int VuoRendererComposition::majorGridLineSpacing = 4*VuoRendererComposition::minorGridLineSpacing;

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
	this->renderHiddenCables = false;
	this->cachingEnabled = enableCaching;
	this->publishedInputNode = createPublishedInputNode();
	this->publishedOutputNode = createPublishedOutputNode();

	parser = NULL;

	signaler = new VuoRendererSignaler();

	// Add any nodes, cables, published ports, and published cables that are already in the base composition.

	set<VuoNode *> nodes = getBase()->getNodes();
	foreach (VuoNode *node, nodes)
		addNodeInCompositionToCanvas(node);

	vector<VuoPublishedPort *> publishedInputPorts = getBase()->getPublishedInputPorts();
	foreach (VuoPublishedPort *publishedPort, publishedInputPorts)
		createRendererForPublishedPortInComposition(publishedPort, true);

	vector<VuoPublishedPort *> publishedOutputPorts = getBase()->getPublishedOutputPorts();
	foreach (VuoPublishedPort *publishedPort, publishedOutputPorts)
		createRendererForPublishedPortInComposition(publishedPort, false);

	set<VuoCable *> cables = getBase()->getCables();
	foreach (VuoCable *cable, cables)
	{
		addCableInCompositionToCanvas(cable);

		if (cable->isPublishedInputCable())
			cable->setFrom(publishedInputNode, cable->getFromPort());
		if (cable->isPublishedOutputCable())
			cable->setTo(publishedOutputNode, cable->getToPort());
	}

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

	if (VuoRendererReadOnlyDictionary::isReadOnlyDictionary(baseNode))
		rn = new VuoRendererReadOnlyDictionary(baseNode, signaler);

	else if (VuoRendererKeyListForReadOnlyDictionary::isKeyListForReadOnlyDictionary(baseNode))
		rn = new VuoRendererKeyListForReadOnlyDictionary(baseNode, signaler);

	else if (VuoRendererValueListForReadOnlyDictionary::isValueListForReadOnlyDictionary(baseNode))
		rn = new VuoRendererValueListForReadOnlyDictionary(baseNode, signaler);

	else if (VuoCompilerMakeListNodeClass::isMakeListNodeClassName(baseNode->getNodeClass()->getClassName()))
		rn = new VuoRendererInputListDrawer(baseNode, signaler);

	else
		rn = new VuoRendererNode(baseNode, signaler);

	return rn;
}

/**
 * Adds a node to the underlying composition and (if `nodeShouldBeRendered`
 * is true), to the canvas.
 *
 * If the node is to be added to the canvas and doesn't have a renderer detail,
 * one is created for it.
 *
 * If a node with the same graphviz identifier as this node is already in the
 * composition, changes the graphviz identifier of this node to be unique.
 */
void VuoRendererComposition::addNode(VuoNode *n, bool nodeShouldBeRendered)
{
	if (getBase()->hasCompiler())
		getBase()->getCompiler()->setUniqueGraphvizIdentifierForNode(n);

	getBase()->addNode(n);

	if (nodeShouldBeRendered)
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

	// The following VuoRendererCable::setFrom()/setTo() calls are unnecessary as far as the
	// base cable is concerned, but forces the connected component renderings to update appropriately.
	rc->setFrom(rc->getBase()->getFromNode(), rc->getBase()->getFromPort());
	rc->setTo(rc->getBase()->getToNode(), rc->getBase()->getToPort());

	// Performance optimizations
	rc->setCacheMode(getCurrentDefaultCacheMode());
}

/**
 * Removes a node from the canvas and the underlying composition.
 */
void VuoRendererComposition::removeNode(VuoRendererNode *rn)
{
	rn->updateGeometry();
	removeItem(rn);

	getBase()->removeNode(rn->getBase());
}

/**
 * Removes a cable from the canvas and the underlying composition.
 */
void VuoRendererComposition::removeCable(VuoRendererCable *rc)
{
	rc->setFrom(NULL, NULL);
	rc->setTo(NULL, NULL);

	rc->updateGeometry();
	rc->removeFromScene();

	getBase()->removeCable(rc->getBase());
}

/**
 * Creates and connects the appropriate input attachments to the provided @c node.
 */
void VuoRendererComposition::createAndConnectInputAttachments(VuoRendererNode *node, VuoCompiler *compiler)
{
	if (node->getBase()->getNodeClass()->getClassName() == "vuo.math.calculate")
		createAndConnectDrawersToReadOnlyDictionaryInputPorts(node, compiler);

	createAndConnectDrawersToListInputPorts(node, compiler);

}

/**
 * Creates and connects a "Make List" drawer to each of the provided node's list input ports.
 */
void VuoRendererComposition::createAndConnectDrawersToListInputPorts(VuoRendererNode *node, VuoCompiler *compiler)
{
	foreach (VuoPort *port, node->getBase()->getInputPorts())
	{
		VuoCompilerInputEventPort *inputEventPort = (port->hasCompiler()? dynamic_cast<VuoCompilerInputEventPort *>(port->getCompiler()) : NULL);
		if (inputEventPort && VuoCompilerType::isListType(inputEventPort->getDataType()))
		{
			VuoRendererCable *cable = NULL;
			VuoRendererNode *makeListNode = createAndConnectMakeListNode(node->getBase(), port, compiler, cable);
			addNode(makeListNode->getBase());
			addCable(cable->getBase());
		}
	}
}

/**
 * Creates and connects the appropriate read-only dictionary attachments to the provided @c node.
 */
void VuoRendererComposition::createAndConnectDrawersToReadOnlyDictionaryInputPorts(VuoRendererNode *node, VuoCompiler *compiler)
{
	set<VuoRendererNode *> nodesToAdd;
	set<VuoRendererCable *> cablesToAdd;
	createAndConnectDictionaryAttachmentsForNode(node->getBase(), compiler, nodesToAdd, cablesToAdd);

	foreach (VuoRendererNode *node, nodesToAdd)
		addNode(node->getBase());

	foreach (VuoRendererCable *cable, cablesToAdd)
		addCable(cable->getBase());
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
 * Creates the nodes and connecting cables that the input @c node will need to provide
 * it with an input dictionary of keys and values, to be attached to the node's "values" input port.
 *
 * @param node The node that needs the dictionary attachments created.
 * @param compiler A compiler used to get the attachment node classes.
 * @param[out] createdNodes The created nodes.
 * @param[out] createdCables The created cables.
 */
void VuoRendererComposition::createAndConnectDictionaryAttachmentsForNode(VuoNode *node,
																		  VuoCompiler *compiler,
																		  set<VuoRendererNode *> &createdNodes,
																		  set<VuoRendererCable *> &createdCables)
{
	createdNodes.clear();
	createdCables.clear();

	VuoPort *expressionInputPort = node->getInputPortWithName("expression");
	VuoPort *valuesInputPort = node->getInputPortWithName("values");
	if (!(expressionInputPort && valuesInputPort))
	{
		VUserLog("Error: Cannot create dictionary attachments for a node without 'expression' and 'values' input ports.");
		return;
	}

	// Assume for now that the dictionary should map strings to reals for use as variables in a VuoMathExpressionList.
	const string dictionaryTypeName = "VuoDictionary_VuoText_VuoReal";
	const string dictionaryClassName = "vuo.dictionary.make.VuoText.VuoReal";
	const string dictionaryKeySourceTypeName = "VuoMathExpressionList";

	VuoCompilerPort *valuesInputPortCompiler = static_cast<VuoCompilerPort *>(valuesInputPort->getCompiler());
	if (valuesInputPortCompiler->getDataVuoType()->getModuleKey() != dictionaryTypeName)
	{
		VUserLog("Error: Unexpected dictionary type required: %s", valuesInputPortCompiler->getDataVuoType()->getModuleKey().c_str());
		return;
	}

	VuoCompilerPort *expressionInputPortCompiler = static_cast<VuoCompilerPort *>(expressionInputPort->getCompiler());
	if (expressionInputPortCompiler->getDataVuoType()->getModuleKey() != dictionaryKeySourceTypeName)
	{
		VUserLog("Error: Unexpected key source type encountered: %s", expressionInputPortCompiler->getDataVuoType()->getModuleKey().c_str());
		return;
	}

	// Extract the variable names from the math expressions.
	VuoCompilerInputEventPort *expressionInputEventPort = static_cast<VuoCompilerInputEventPort *>(expressionInputPortCompiler);
	string expressionConstant = expressionInputEventPort->getData()->getInitialValue();
	vector<string> inputVariables = extractInputVariableListFromExpressionsConstant(expressionConstant);
	unsigned long itemCount = inputVariables.size();

	string keyListClassName = VuoCompilerMakeListNodeClass::buildNodeClassName(itemCount, "VuoText");
	string valueListClassName = VuoCompilerMakeListNodeClass::buildNodeClassName(itemCount, "VuoReal");

	VuoCompilerNodeClass *keyListNodeClass = compiler->getNodeClass(keyListClassName);
	VuoCompilerNodeClass *valueListNodeClass = compiler->getNodeClass(valueListClassName);
	VuoCompilerNodeClass *dictionaryNodeClass = compiler->getNodeClass(dictionaryClassName);

	if (keyListNodeClass && valueListNodeClass && dictionaryNodeClass)
	{
		// Create and connect all base components before creating any renderer components so that createRendererNode()
		// has all of the information it needs to create the appropriate renderer form for each node.
		QPoint offset(-220, 50);
		VuoNode *dictionaryNode = compiler->createNode(dictionaryNodeClass, "", node->getX()+offset.x(), node->getY()+offset.y());
		VuoNode *keyListNode = compiler->createNode(keyListNodeClass, "", node->getX()+offset.x(), node->getY()+offset.y());
		VuoNode *valueListNode = compiler->createNode(valueListNodeClass, "", node->getX()+offset.x(), node->getY()+offset.y());

		VuoCable *cableCarryingDictionary = (new VuoCompilerCable(dictionaryNode->getCompiler(),
														   static_cast<VuoCompilerPort *>(dictionaryNode->getOutputPortWithName("dictionary")->getCompiler()),
														   node->getCompiler(),
														   static_cast<VuoCompilerPort *>(valuesInputPort->getCompiler())))->getBase();

		VuoCable *cableCarryingKeys = (new VuoCompilerCable(keyListNode->getCompiler(),
														   static_cast<VuoCompilerPort *>(keyListNode->getOutputPortWithName("list")->getCompiler()),
														   dictionaryNode->getCompiler(),
														   static_cast<VuoCompilerPort *>(dictionaryNode->getInputPortWithName("keys")->getCompiler())))->getBase();

		VuoCable *cableCarryingValues = (new VuoCompilerCable(valueListNode->getCompiler(),
														   static_cast<VuoCompilerPort *>(valueListNode->getOutputPortWithName("list")->getCompiler()),
														   dictionaryNode->getCompiler(),
														   static_cast<VuoCompilerPort *>(dictionaryNode->getInputPortWithName("values")->getCompiler())))->getBase();

		// Set the variable names extracted from the math expressions.
		vector<VuoPort *> keyPorts = keyListNode->getInputPorts();
		for (size_t i = 0; i < itemCount; ++i)
		{
			VuoPort *keyPort = keyPorts[i + VuoNodeClass::unreservedInputPortStartIndex];
			string key = "\"" + inputVariables[i] + "\"";

			VuoCompilerInputEventPort *keyEventPort = static_cast<VuoCompilerInputEventPort *>(keyPort->getCompiler());
			keyEventPort->getData()->setInitialValue(key);
		}

		createdNodes.insert(createRendererNode(dictionaryNode));
		createdNodes.insert(createRendererNode(keyListNode));
		createdNodes.insert(createRendererNode(valueListNode));

		createdCables.insert(new VuoRendererCable(cableCarryingDictionary));
		createdCables.insert(new VuoRendererCable(cableCarryingKeys));
		createdCables.insert(new VuoRendererCable(cableCarryingValues));
	}
}

/**
 * Extracts the input variables from the provided "inputVariables" @c constant
 * and returns the variables in an ordered list.
 */
vector<string> VuoRendererComposition::extractInputVariableListFromExpressionsConstant(string constant)
{
	vector<string> inputVariables;

	json_object *js = json_tokener_parse(constant.c_str());
	json_object *expressionsObject = NULL;

	if (json_object_object_get_ex(js, "inputVariables", &expressionsObject))
	{
		if (json_object_get_type(expressionsObject) == json_type_array)
		{
			int variableCount = json_object_array_length(expressionsObject);
			for (int i = 0; i < variableCount; ++i)
			{
				json_object *itemObject = json_object_array_get_idx(expressionsObject, i);
				if (json_object_get_type(itemObject) == json_type_string)
					inputVariables.push_back(json_object_get_string(itemObject));
			}
		}
	}
	json_object_put(js);

	return inputVariables;
}

/**
 * Creates a renderer detail for the pre-existing @c publishedPort, on the assumption that
 * the published port provided already exists in the base composition and has an associated compiler detail.
 */
VuoRendererPublishedPort * VuoRendererComposition::createRendererForPublishedPortInComposition(VuoPublishedPort *publishedPort, bool isPublishedInput)
{
	if (! publishedPort->hasCompiler())
		return NULL;

	return new VuoRendererPublishedPort(publishedPort, !isPublishedInput);
}

/**
 * Adds an existing VuoPublishedPort as one of this composition's published ports.
 */
void VuoRendererComposition::addPublishedPort(VuoPublishedPort *publishedPort, bool isPublishedInput, VuoCompiler *compiler)
{
	string name = publishedPort->getClass()->getName();
	if (isPublishedInput)
	{
		VuoPublishedPort *existingPort = getBase()->getPublishedInputPortWithName(name);
		if (! existingPort)
		{
			int index = getBase()->getPublishedInputPorts().size();
			getBase()->addPublishedInputPort(publishedPort, index);
		}
		else if (publishedPort != existingPort)
			VUserLog("Error: Unhandled published port name conflict.");
	}
	else // if (! isPublishedInput)
	{
		VuoPublishedPort *existingPort = getBase()->getPublishedOutputPortWithName(name);
		if (! existingPort)
		{
			int index = getBase()->getPublishedOutputPorts().size();
			getBase()->addPublishedOutputPort(publishedPort, index);
		}
		else if (publishedPort != existingPort)
			VUserLog("Error: Unhandled published port name conflict.");
	}
}

/**
 * Removes a published input or output VuoRendererPublishedPort from the list
 * of published ports associated with this composition.
 *
 * @return The index within the list of published input port output ports at which the port was located, or -1 if not located.
 */
int VuoRendererComposition::removePublishedPort(VuoPublishedPort *publishedPort, bool isPublishedInput, VuoCompiler *compiler)
{
	if (isPublishedInput)
	{
		int index = getBase()->getIndexOfPublishedPort(publishedPort, isPublishedInput);
		if (index != -1)
			getBase()->removePublishedInputPort(index);

		return index;
	}
	else
	{
		int index = getBase()->getIndexOfPublishedPort(publishedPort, isPublishedInput);
		if (index != -1)
			getBase()->removePublishedOutputPort(index);

		return index;
	}
}

/**
 * Sets the name of the provided @c publishedPort to @c name; updates the composition's
 * published pseudo-node and connected published cables accordingly.
 */
void VuoRendererComposition::setPublishedPortName(VuoRendererPublishedPort *publishedPort, string name, VuoCompiler *compiler)
{
	bool isPublishedInput = !publishedPort->getInput();
	publishedPort->setName(getUniquePublishedPortName(name, isPublishedInput));
}

/**
 * Creates and returns a dummy published input node for this composition.
 * Its port classes are unpopulated not expected to be maintained
 * as the composition's set of published ports changes.
 */
VuoNode * VuoRendererComposition::createPublishedInputNode()
{
	VuoNodeClass *dummyPublishedInputNodeClass = new VuoNodeClass(VuoNodeClass::publishedInputNodeClassName, vector<string>(), vector<string>());
	return dummyPublishedInputNodeClass->newNode(VuoNodeClass::publishedInputNodeIdentifier);
}

/**
 * Creates and returns a dummy published output node for this composition.
 * Its port classes are unpopulated not expected to be maintained
 * as the composition's set of published ports changes.
 */
VuoNode * VuoRendererComposition::createPublishedOutputNode()
{
	VuoNodeClass *dummyPublishedOutputNodeClass = new VuoNodeClass(VuoNodeClass::publishedOutputNodeClassName, vector<string>(), vector<string>());
	return dummyPublishedOutputNodeClass->newNode(VuoNodeClass::publishedOutputNodeIdentifier);
}

/**
 * Returns a string derived from the input @c baseName that is guaranteed
 * to be unique either among the published input port names or among the published
 * output port names for this composition, as specified by @c isInput.
 */
string VuoRendererComposition::getUniquePublishedPortName(string baseName, bool isPublishedInput)
{
	string uniquePortName = baseName;
	string uniquePortNamePrefix = uniquePortName;
	int portNameInstanceNum = 1;
	while (isPublishedPortNameTaken(uniquePortName, isPublishedInput) || uniquePortName.empty())
	{
		ostringstream oss;
		oss << ++portNameInstanceNum;
		uniquePortName = uniquePortNamePrefix + oss.str();
	}

	return uniquePortName;
}

/**
 * Returns a boolean indicating whether the input @c name is already taken
 * either by a published input port or by a published output port associated with this
 * composition, as specified by @c isInput.
 */
bool VuoRendererComposition::isPublishedPortNameTaken(string name, bool isPublishedInput)
{
	if (name == "refresh")
		return true;

	VuoPublishedPort *publishedPort = (isPublishedInput ?
										   getBase()->getPublishedInputPortWithName(name) :
										   getBase()->getPublishedOutputPortWithName(name));
	return (publishedPort != NULL);
}

/**
 * Returns true if the internal @c port is connected to a published port.
 */
bool VuoRendererComposition::isPortPublished(VuoRendererPort *port)
{
	return (!port->getPublishedPorts().empty());
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

	// Don't try to collapse typecast nodes outputting to multiple nodes, or without any output cables.
	if (outCables.size() != 1)
		return NULL;

	VuoCable *outCable = *(outCables.begin());

	// Don't try to collapse typecast nodes whose outgoing cable is event-only.
	if (!(outCable->hasRenderer() && outCable->getRenderer()->effectivelyCarriesData()))
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
	if (outCable->isPublished())
		return NULL;

	VuoNode *fromNode = incomingDataCable->getFromNode();
	VuoPort *fromPort = incomingDataCable->getFromPort();

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

	// Don't try to collapse typecast nodes outputting to internal ports that have been published.
	if (isPortPublished(toPort->getRenderer()))
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

	QGraphicsItem::CacheMode defaultCacheMode = getCurrentDefaultCacheMode();
	foreach (VuoCable *cable, inCables)
	{
		if (cable->hasRenderer())
		{
			cable->getRenderer()->setCacheMode(QGraphicsItem::NoCache);
			cable->getRenderer()->updateGeometry();
			cable->getRenderer()->setCacheMode(defaultCacheMode);
		}
	}

	foreach (VuoCable *cable, outCables)
	{
		if (cable->hasRenderer())
		{
			cable->getRenderer()->setCacheMode(QGraphicsItem::NoCache);
			cable->getRenderer()->updateGeometry();
			cable->getRenderer()->setCacheMode(defaultCacheMode);
		}
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

	QGraphicsItem::CacheMode defaultCacheMode = getCurrentDefaultCacheMode();
	foreach (VuoCable *cable, typecastInPort->getConnectedCables(true))
	{
		if (cable->hasRenderer())
		{
			cable->getRenderer()->setCacheMode(QGraphicsItem::NoCache);
			cable->getRenderer()->updateGeometry();
			cable->getRenderer()->setCacheMode(defaultCacheMode);
		}
	}

	foreach (VuoCable *cable, typecastOutPort->getConnectedCables(true))
	{
		if (cable->hasRenderer())
		{
			cable->getRenderer()->setCacheMode(QGraphicsItem::NoCache);
			cable->getRenderer()->updateGeometry();
			cable->getRenderer()->setCacheMode(defaultCacheMode);
		}
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
 * Returns the published input node associated with this composition.
 */
VuoNode * VuoRendererComposition::getPublishedInputNode()
{
	return this->publishedInputNode;
}

/**
 * Returns the published output node associated with this composition.
 */
VuoNode * VuoRendererComposition::getPublishedOutputNode()
{
	return this->publishedOutputNode;
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
void VuoRendererComposition::updateGeometryForAllComponents()
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
 * Draws the background of the scene using `painter`, before any items
 * and the foreground are drawn.
 *
 * Reimplementation of QGraphicsScene::drawBackground(QPainter *painter, const QRectF &rect).
 */
void VuoRendererComposition::drawBackground(QPainter *painter, const QRectF &rect)
{
	QGraphicsScene::drawBackground(painter, rect);

	// Draw grid lines.
	if (VuoRendererComposition::gridLineOpacity > 0)
	{
		int gridSpacing = VuoRendererComposition::majorGridLineSpacing;

		qreal leftmostGridLine = quantizeToNearestGridLine(rect.topLeft(), gridSpacing).x();
		if (leftmostGridLine < rect.left())
			leftmostGridLine += gridSpacing;
		qreal topmostGridLine = quantizeToNearestGridLine(rect.topLeft(), gridSpacing).y();
		if (topmostGridLine < rect.top())
			topmostGridLine += gridSpacing;

		// Correct for the fact that VuoRendererNode::paint() starts painting at (-1,0) rather than (0,0).
		// @todo: Eliminate this correction after modifying VuoRendererNode::paint()
		// for https://b33p.net/kosada/node/10210 .
		const int nodeXAlignmentCorrection = -1;

		QVector<QLineF> gridLines;
		for (qreal x = leftmostGridLine; x < rect.right(); x += gridSpacing)
			gridLines.append(QLineF(x + nodeXAlignmentCorrection, rect.top(), x + nodeXAlignmentCorrection, rect.bottom()));
		for (qreal y = topmostGridLine; y < rect.bottom(); y += gridSpacing)
			gridLines.append(QLineF(rect.left(), y, rect.right(), y));

		painter->setPen(QColor(128, 128, 128, VuoRendererComposition::gridLineOpacity));
		painter->drawLines(gridLines);
	}
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
	}

	updateGeometryForAllComponents();
	setComponentCaching(getCurrentDefaultCacheMode());
}

/**
 * Returns a boolean indicating whether hidden cables within this composition are currently displayed.
 */
bool VuoRendererComposition::getRenderHiddenCables()
{
	return this->renderHiddenCables;
}

/**
 * Sets the boolean indicating whether hidden cables within this composition are currently displayed.
 */
void VuoRendererComposition::setRenderHiddenCables(bool render)
{
	setComponentCaching(QGraphicsItem::NoCache);
	updateGeometryForAllComponents();
	this->renderHiddenCables = render;
	setComponentCaching(getCurrentDefaultCacheMode());
}

/**
 * Sets the caching mode for each applicable graphics item within the
 * composition to the provided cache mode.
 */
void VuoRendererComposition::setComponentCaching(QGraphicsItem::CacheMode cacheMode)
{
	// Nodes and ports
	foreach (VuoNode *node, getBase()->getNodes())
	{
		VuoRendererNode *rn = node->getRenderer();
		if (rn)
			rn->setCacheModeForNodeAndPorts(cacheMode);
	}

	// Cables
	set<VuoCable *> allCables = getBase()->getCables();
	foreach (VuoCable *cable, allCables)
	{
		VuoRendererCable *rc = cable->getRenderer();
		if (rc)
			rc->setCacheMode(cacheMode);
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
	return (getBase()->hasCompiler()? getBase()->getCompiler()->getGraphvizDeclaration() : "");
}

/**
 * Exports the composition as an OS X .app bundle.
 *
 * @param[in] savePath The path where the .app is to be saved.
 * @param[in] compiler The compiler to be used to generate the composition executable.
 * @param[out] errString The error message resulting from the export process, if any.
 * @param[in] driver The protocol driver that should be applied to the exported composition. May be NULL.
 * @return An @c appExportResult value detailing the outcome of the export attempt.
 */
VuoRendererComposition::appExportResult VuoRendererComposition::exportApp(const QString &savePath, VuoCompiler *compiler, string &errString, VuoCompilerDriver *driver)
{
	// Set up the directory structure for the app bundle in a temporary location.
	string tmpAppPath = createAppBundleDirectoryStructure();

	// Generate and bundle the composition executable.
	string dir, file, ext;
	VuoFileUtilities::splitPath(savePath.toUtf8().constData(), dir, file, ext);
	string buildErrString = "";
	if (!bundleExecutable(compiler, tmpAppPath + "/Contents/MacOS/" + file, buildErrString, driver))
	{
		errString = buildErrString;
		return exportBuildFailure;
	}

	// Generate and bundle the Info.plist.
	string plist = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"><plist version=\"1.0\"><dict>";
	plist += "<key>NSHighResolutionCapable</key><true/>";
	plist += "<key>CFBundleExecutable</key><string>" + file + "</string>";
	plist += "<key>CFBundleIconFile</key><string>" + file + ".icns</string>";
	plist += "<key>ATSApplicationFontsPath</key><string>Fonts</string>";
	plist += "</dict></plist>";
	VuoFileUtilities::writeStringToFile(plist, tmpAppPath + "/Contents/Info.plist");

	// Bundle resource files referenced within the composition by relative file paths.
	bundleResourceFiles(tmpAppPath + "/Contents/Resources/");

	// Copy the default app icon if one has not been bundled already.
	QString bundledIconPath = QString::fromStdString(tmpAppPath + "/Contents/Resources/" + file + ".icns");
	if (!QFile(bundledIconPath).exists())
		QFile::copy("/System/Library/CoreServices/CoreTypes.bundle/Contents/Resources/GenericApplicationIcon.icns", bundledIconPath);

	// Bundle the essential components of Vuo.framework.
	string sourceVuoFrameworkPath = VuoFileUtilities::getVuoFrameworkPath();
	string targetVuoFrameworkPath = tmpAppPath + "/Contents/Frameworks/Vuo.framework/Versions/" + VUO_VERSION_STRING;
	bundleVuoSubframeworks(sourceVuoFrameworkPath, targetVuoFrameworkPath);
	bundleVuoFrameworkFolder(sourceVuoFrameworkPath + "/Modules", targetVuoFrameworkPath + "/Modules", "dylib");
	bundleVuoFrameworkFolder(sourceVuoFrameworkPath + "/Documentation/Licenses", targetVuoFrameworkPath + "/Documentation/Licenses");

	// Move any pre-existing app of the same name to a backup location.
	bool nameConflict = VuoFileUtilities::fileExists(savePath.toUtf8().constData());
	string backedUpAppPath = "";
	if (nameConflict)
	{
		string dir, file, ext;
		VuoFileUtilities::splitPath(savePath.toUtf8().constData(), dir, file, ext);
		backedUpAppPath = VuoFileUtilities::makeTmpDir(file);

		rename(savePath.toUtf8().constData(), backedUpAppPath.c_str());
	}

	// Move the generated app bundle to the desired save path.
	bool saveSucceeded = (! rename(tmpAppPath.c_str(), savePath.toUtf8().constData()));

	if (!saveSucceeded)
	{
		// If the new app couldn't be saved in the target location for some reason,
		// try to restore the backed-up app (if any).
		if (nameConflict)
			rename(backedUpAppPath.c_str(), savePath.toUtf8().constData());

		return exportSaveFailure;
	}

	else
	{
		// If the export succeeded, delete the backed-up app (if any).
		if (nameConflict)
			QDir(backedUpAppPath.c_str()).removeRecursively();

		return exportSuccess;
	}
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

	string resourcesFontsPath = contentsPath + "/Resources/Fonts";
	mkdir(resourcesFontsPath.c_str(), 0755);

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

	string vuoFrameworksPathVersionsCurrentDocumentation = vuoFrameworksPathVersionsCurrent + "/Documentation";
	mkdir(vuoFrameworksPathVersionsCurrentDocumentation.c_str(), 0755);

	string vuoFrameworksPathVersionsCurrentLicenses = vuoFrameworksPathVersionsCurrent + "/Documentation/Licenses";
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
bool VuoRendererComposition::bundleExecutable(VuoCompiler *compiler, string targetExecutablePath, string &errString, VuoCompilerDriver *driver)
{
	// Generate the executable.
	try
	{
		VuoCompilerComposition *compiledCompositionToExport = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(takeSnapshot(), compiler);
		if (driver)
			driver->applyToComposition(compiledCompositionToExport);

		// Modify port constants that contain relative paths so that the paths will be
		// resolved correctly relative to the "Resources" directory within the app bundle.
		VuoRendererComposition *rendererCompositionToExport = new VuoRendererComposition(VuoCompilerComposition::newCompositionFromGraphvizDeclaration(compiledCompositionToExport->getGraphvizDeclaration(), compiler)->getBase());
		rendererCompositionToExport->modifyAllRelativeResourcePathsForAppBundle();
		delete rendererCompositionToExport;

		string pathOfCompiledCompositionToExport = VuoFileUtilities::makeTmpFile(compiledCompositionToExport->getBase()->getName(), "bc");

		compiler->compileComposition(compiledCompositionToExport, pathOfCompiledCompositionToExport);

		string rPath = "@loader_path/../Frameworks";
		compiler->linkCompositionToCreateExecutable(pathOfCompiledCompositionToExport, targetExecutablePath, VuoCompiler::Optimization_SmallBinary, true, rPath);
		remove(pathOfCompiledCompositionToExport.c_str());
	}

	catch (const exception &e)
	{
		errString = e.what();
		return false;
	}

	return true;
}

/**
 * Maps all relative resource paths referenced within this composition's port constants
 * to the corresponding paths appropriate for use within the "Resources" directory of an
 * exported app bundle.
 */
void VuoRendererComposition::modifyAllRelativeResourcePathsForAppBundle()
{
	foreach (VuoNode *node, getBase()->getNodes())
	{
		foreach (VuoPort *port, node->getInputPorts())
		{
			if (hasRelativeReadURLConstantValue(port))
			{
				VuoUrl url = VuoUrl_makeFromString(port->getRenderer()->getConstantAsString().c_str());
				VuoRetain(url);
				string modifiedRelativeResourcePath = modifyResourcePathForAppBundle(url);
				port->getRenderer()->setConstant("\"" + modifiedRelativeResourcePath + "\"");
				VuoRelease(url);
			}
		}
	}
}

/**
 * Maps all relative resource paths referenced within this composition's port constants
 * to the corresponding paths appropriate for use after the composition has been
 * relocated to `newDir`.
 */
map<VuoPort *, string> VuoRendererComposition::getResourcePathsRelativeToDir(QDir newDir)
{
	map<VuoPort *, string> modifiedPathForPort;
	foreach (VuoNode *node, getBase()->getNodes())
	{
		foreach (VuoPort *port, node->getInputPorts())
		{
			if (hasRelativeReadURLConstantValue(port))
			{
				VuoUrl url = VuoUrl_makeFromString(port->getRenderer()->getConstantAsString().c_str());
				VuoRetain(url);
				QString origRelativeResourcePath = url;
				string modifiedRelativeResourcePath = modifyResourcePathForNewDir(origRelativeResourcePath.toUtf8().constData(), newDir);

				if (modifiedRelativeResourcePath != origRelativeResourcePath.toUtf8().constData())
					modifiedPathForPort[port] = "\"" + modifiedRelativeResourcePath + "\"";
				VuoRelease(url);
			}
		}
	}

	return modifiedPathForPort;
}

/**
 * Copies resources referenced within the composition by relative URL into the
 * provided @c targetResourceDir.
 *
 * If `tmpFilesOnly` is `true`, bundles only files that are originally located
 * within the `/tmp` directory or one of its subdirectories and ignores all others.
 *
 * Helper function for VuoRendererComposition::exportApp(const QString &savePath) and
 * VuoEditorWindow::on_saveCompositionAs_triggered().
 */
void VuoRendererComposition::bundleResourceFiles(string targetResourceDir, bool tmpFilesOnly)
{
	foreach (VuoNode *node, getBase()->getNodes())
	{
		foreach (VuoPort *port, node->getInputPorts())
		{
			if (hasRelativeReadURLConstantValue(port))
			{
				VuoUrl url = VuoUrl_makeFromString(port->getRenderer()->getConstantAsString().c_str());
				VuoRetain(url);
				QString origRelativeResourcePath = url;
				QString modifiedRelativeResourcePath = modifyResourcePathForAppBundle(origRelativeResourcePath.toUtf8().constData()).c_str();

				string origRelativeDir, modifiedRelativeDir, file, ext;
				VuoFileUtilities::splitPath(origRelativeResourcePath.toUtf8().constData(), origRelativeDir, file, ext);
				VuoFileUtilities::splitPath(modifiedRelativeResourcePath.toUtf8().constData(), modifiedRelativeDir, file, ext);
				string resourceFileName = file;
				if (!ext.empty())
				{
					resourceFileName += ".";
					resourceFileName += ext;
				}

				QDir compositionDir(QDir(getBase()->getDirectory().c_str()).canonicalPath());
				QDir appDir(QDir(targetResourceDir.c_str()).canonicalPath());

				QString sourceFilePath = compositionDir.filePath(QDir(origRelativeDir.c_str()).filePath(resourceFileName.c_str()));
				if (!tmpFilesOnly || isTmpFile(sourceFilePath.toUtf8().constData()))
				{
					if (!modifiedRelativeDir.empty())
						appDir.mkpath(modifiedRelativeDir.c_str());

					QString targetFilePath = appDir.filePath(QDir(modifiedRelativeDir.c_str()).filePath(resourceFileName.c_str()));
					copyFileOrDirectory(sourceFilePath.toUtf8().constData(), targetFilePath.toUtf8().constData());

					if (isSupportedSceneFile(sourceFilePath.toUtf8().constData()))
						bundleAuxiliaryFilesForSceneFile(sourceFilePath, targetFilePath);
				}
				VuoRelease(url);
			}

		}
	}

	// @todo https://b33p.net/kosada/node/9205 : Published input port constants?
}

/**
 * Returns a boolean indicating whether the provided `filePath` is within the
 * `/tmp` directory (or a subdirectory thereof).
 */
bool VuoRendererComposition::isTmpFile(string filePath)
{
	const QString tmpDirPath = QDir(VuoFileUtilities::getTmpDir().c_str()).canonicalPath();
	QDir parentDir(QDir(QFileInfo(filePath.c_str()).dir()).canonicalPath());

	do
	{
		if (parentDir.canonicalPath() == tmpDirPath)
			return true;
	} while (parentDir.cdUp());

	return false;
}

/**
 * Given a resource @c path, returns the corresponding mapped path
 * to be used within the "Resources" directory of an exported app bundle.
 */
string VuoRendererComposition::modifyResourcePathForAppBundle(string path)
{
	if (VuoUrl_isRelativePath(path.c_str()))
	{
		// Replace parent-directory indicators ("../") so that resources
		// located within the parent (or ancestor) directory of the original composition will
		// still be copied into the exported app's "Resources" directory or a subdirectory thereof.
		const QString originalParentDirIndicator = "/../";
		const QString appBundleParentDirIndicator = "/VuoParentDir/";

		QString modifiedPath = QString("/").append(path.c_str());

		while (modifiedPath.contains(originalParentDirIndicator))
			modifiedPath.replace(originalParentDirIndicator, appBundleParentDirIndicator);

		modifiedPath.remove(0, 1); // Remove leading '/' added earlier

		return modifiedPath.toUtf8().constData();
	}

	else
		return path;
}

/**
 * Given a resource @c path, returns the corresponding mapped path
 * to be used after the composition has been relocated to `newDir`.
 */
string VuoRendererComposition::modifyResourcePathForNewDir(string path, QDir newDir)
{
	if (VuoUrl_isRelativePath(path.c_str()))
	{
		QDir compositionDir(QDir(getBase()->getDirectory().c_str()).canonicalPath());
		if (compositionDir.canonicalPath() == newDir.canonicalPath())
			return path;

		string origRelativeDir, file, ext;
		VuoFileUtilities::splitPath(path, origRelativeDir, file, ext);
		string resourceFileName = file;
		if (!ext.empty())
		{
			resourceFileName += ".";
			resourceFileName += ext;
		}

		QString sourceFilePath = compositionDir.filePath(QDir(origRelativeDir.c_str()).filePath(resourceFileName.c_str()));
		string modifiedPath = newDir.relativeFilePath(sourceFilePath).toUtf8().constData();

		return modifiedPath;
	}

	else
		return path;
}

/**
 * Recursively copies the provided file or directory from @c sourcePath to @c targetPath.
 */
void VuoRendererComposition::copyFileOrDirectory(string sourcePath, string targetPath)
{
	QFileInfo sourceFileInfo = QFileInfo(sourcePath.c_str());

	if (sourceFileInfo.isFile())
		QFile::copy(sourcePath.c_str(),targetPath.c_str());

	else if (sourceFileInfo.isDir())
	{
		QDir sourceDir(sourcePath.c_str());
		QDir targetDir(targetPath.c_str());
		targetDir.mkpath(".");
		foreach (QString file, sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
		{
			QString sourceFile = QString(sourcePath.c_str()) + QDir::separator() + file;
			QString targetFile = QString(targetPath.c_str()) + QDir::separator() + file;
			copyFileOrDirectory(sourceFile.toUtf8().constData(), targetFile.toUtf8().constData());
		}
	}
}

/**
 * Uses a heuristic to locate and bundle texture files that may be required by the
 * provided mesh file having original path `sourceFilePath` and bundled path `targetFilePath`.
 */
void VuoRendererComposition::bundleAuxiliaryFilesForSceneFile(QString sourceFilePath, QString targetFilePath)
{
	string sourceDirName, targetDirName, file, ext;
	VuoFileUtilities::splitPath(sourceFilePath.toUtf8().constData(), sourceDirName, file, ext);
	VuoFileUtilities::splitPath(targetFilePath.toUtf8().constData(), targetDirName, file, ext);

	// Bundle any file or folder in the same directory as the mesh file
	// whose name begins with the mesh file's basename.
	QDir sourceDir(sourceDirName.c_str());
	QStringList filesWithMatchingBaseName = QStringList() << QString(file.c_str()).append("*");
	sourceDir.setNameFilters(filesWithMatchingBaseName);

	foreach (QString auxiliaryFile, sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
	{
		QString sourceFile = QString(sourceDirName.c_str()) + QDir::separator() + auxiliaryFile;
		QString targetFile = QString(targetDirName.c_str()) + QDir::separator() + auxiliaryFile;
		if (!QFileInfo(targetFile).exists())
			copyFileOrDirectory(sourceFile.toUtf8().constData(), targetFile.toUtf8().constData());
	}

	// Bundle texture folders in the same directory as the mesh file.
	QStringList textureFolderNames = QStringList() << "Textures" << "_Textures";
	sourceDir.setNameFilters(textureFolderNames);
	foreach (QString textureFolderName, sourceDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
	{
		QString sourceTextureDir = QString(sourceDirName.c_str()) + QDir::separator() + textureFolderName;
		QString targetTextureDir = QString(targetDirName.c_str()) + QDir::separator() + textureFolderName;
		if (!QFileInfo(targetTextureDir).exists())
			copyFileOrDirectory(sourceTextureDir.toUtf8().constData(), targetTextureDir.toUtf8().constData());
	}

	// @todo: Bundle texture folders in the parent directory of the mesh file.
	// See https://b33p.net/kosada/node/9390, https://b33p.net/kosada/node/9391.

	return;
}

/**
 * Returns a boolean indicating whether the provided @c port currently has
 * a relative input file path as a constant value.
 *
 * Returns `false` if the port has an output file path -- a URL that will be written to
 * rather than read from, as indicated by the port's `isSave:true` port detail.
 *
 * Helper function for VuoRendererComposition::bundleResourceFiles(string targetResourceDir).
 */
bool VuoRendererComposition::hasRelativeReadURLConstantValue(VuoPort *port)
{
	if (!(port->hasRenderer() && port->getRenderer()->isConstant() && hasURLType(port)))
		return false;

	json_object *details = static_cast<VuoCompilerInputEventPortClass *>(port->getClass()->getCompiler())->getDataClass()->getDetails();
	json_object *isSaveValue = NULL;
	if (details && json_object_object_get_ex(details, "isSave", &isSaveValue) && json_object_get_boolean(isSaveValue))
		return false;

	VuoUrl url = VuoUrl_makeFromString(port->getRenderer()->getConstantAsString().c_str());
	VuoRetain(url);
	bool isRelativePath = VuoUrl_isRelativePath(url);
	VuoRelease(url);
	return isRelativePath;
}

/**
 * Returns a boolean indicating whether the provided @c port expects a URL as input.
 * @todo https://b33p.net/kosada/node/9204 Just check whether it has a VuoUrl type.
 * For now, use hard-coded rules.
 *
 * Helper function for VuoRendererComposition::hasRelativeReadURLConstantValue(VuoPort *port).
 */
bool VuoRendererComposition::hasURLType(VuoPort *port)
{
	// For now, ports with URLs are expected to be of type "VuoText".
	if (!(port->hasRenderer() &&
		  port->getRenderer()->getDataType() &&
		  port->getRenderer()->getDataType()->getModuleKey()=="VuoText"))
		return false;


	// Case: Port is titled "url"
	if (port->getClass()->getName() == "url")
		return true;

	// Case: Port is an input port on a drawer attached to a port titled "urls"
	// Relevant for vuo.image.fetch.list, vuo.scene.fetch.list nodes.
	VuoRendererInputDrawer *drawer = dynamic_cast<VuoRendererInputDrawer *>(port->getRenderer()->getRenderedParentNode());
	if (drawer)
	{
		VuoPort *hostPort = drawer->getRenderedHostPort();
		if (hostPort && (hostPort->getClass()->getName() == "urls"))
			return true;
	}

	// Case: Port is titled "folder"
	// Relevant for vuo.file.list node.
	if (port->getClass()->getName() == "folder")
	{
		return true;
	}

	return false;
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
	subframeworksToExclude.insert("clang.framework");
	subframeworksToExclude.insert("llvm.framework");

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

/**
 * Returns a boolean indicating whether the file at the provided @c path is
 * a supported audio file.
 */
bool VuoRendererComposition::isSupportedAudioFile(string path)
{
	return VuoFileFormat_isSupportedAudioFile(path.c_str());
}

/**
 * Returns a boolean indicating whether the file at the provided @c path is
 * a supported image file.
 */
bool VuoRendererComposition::isSupportedImageFile(string path)
{
	return VuoFileFormat_isSupportedImageFile(path.c_str());
}

/**
 * Returns a boolean indicating whether the file at the provided @c path is
 * a supported mesh file.
 */
bool VuoRendererComposition::isSupportedMeshFile(string path)
{
	return VuoFileFormat_isSupportedMeshFile(path.c_str());
}

/**
 * Returns a boolean indicating whether the file at the provided @c path is
 * a supported movie file.
 */
bool VuoRendererComposition::isSupportedMovieFile(string path)
{
	return VuoFileFormat_isSupportedMovieFile(path.c_str());
}

/**
 * Returns a boolean indicating whether the file at the provided @c path is
 * a supported scene file.
 */
bool VuoRendererComposition::isSupportedSceneFile(string path)
{
	return VuoFileFormat_isSupportedSceneFile(path.c_str());
}

/**
 * Returns a boolean indicating whether the file at the provided @c path is
 * a supported feed file.
 */
bool VuoRendererComposition::isSupportedFeedFile(string path)
{
	return VuoFileFormat_isSupportedFeedFile(path.c_str());
}

/**
 * Returns a boolean indicating whether the file at the provided @c path is
 * a supported data file.
 */
bool VuoRendererComposition::isSupportedDataFile(string path)
{
	return VuoFileFormat_isSupportedDataFile(path.c_str());
}

/**
 * Specifies the opacity at which grid lines should be rendered on the canvas.
 */
void VuoRendererComposition::setGridLineOpacity(int opacity)
{
	VuoRendererComposition::gridLineOpacity = opacity;
}

/**
 * Returns the opacity at which grid lines should be rendered on the canvas.
 */
int VuoRendererComposition::getGridLineOpacity()
{
	return VuoRendererComposition::gridLineOpacity;
}


/**
 * Quantizes the provided `point` to the nearest horizontal and vertical gridlines
 * with the `gridSpacing` specified.
 */
QPoint VuoRendererComposition::quantizeToNearestGridLine(QPointF point, int gridSpacing)
{
	return QPoint(floor((point.x()/(1.0*gridSpacing))+0.5)*gridSpacing,
				  floor((point.y()/(1.0*gridSpacing))+0.5)*gridSpacing);
}

/**
 * Returns the default composition description.
 */
string VuoRendererComposition::getDefaultCompositionDescription()
{
	return "This composition does...";
}
