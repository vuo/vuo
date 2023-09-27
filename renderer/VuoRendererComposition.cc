/**
 * @file
 * VuoRendererComposition implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoFileType.h"

#include "VuoRendererComposition.hh"

#include "VuoCompiler.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerDriver.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerIssue.hh"
#include "VuoCompilerMakeListNodeClass.hh"
#include "VuoCompilerType.hh"
#include "VuoComment.hh"
#include "VuoComposition.hh"
#include "VuoCompositionMetadata.hh"
#include "VuoNodeClass.hh"
#include "VuoRendererComment.hh"
#include "VuoRendererInputListDrawer.hh"
#include "VuoRendererReadOnlyDictionary.hh"
#include "VuoRendererValueListForReadOnlyDictionary.hh"
#include "VuoRendererSignaler.hh"
#include "VuoRendererTypecastPort.hh"
#include "VuoRendererFonts.hh"
#include "VuoRendererKeyListForReadOnlyDictionary.hh"

#include "VuoStringUtilities.hh"

#include "VuoHeap.h"
#include "type.h"

#include "VuoTextHtml.h"
#include "VuoUrl.h"

#ifdef __APPLE__
#include <objc/message.h>
#endif

int VuoRendererComposition::gridOpacity                           = 0;
VuoRendererComposition::GridType VuoRendererComposition::gridType = VuoRendererComposition::NoGrid;
const int VuoRendererComposition::minorGridLineSpacing            = 15;  // VuoRendererPort::portSpacing
const int VuoRendererComposition::majorGridLineSpacing            = 4 * VuoRendererComposition::minorGridLineSpacing;
const string VuoRendererComposition::deprecatedDefaultDescription = "This composition does...";

/**
 * Creates a canvas upon which nodes and cables can be rendered.
 * The canvas initially contains the nodes, cables, and published ports in the base composition.
 *
 * @param baseComposition The VuoComposition to which the new VuoRendererComposition detail class should be attached.
 * @param renderMissingAsPresent Sets whether node classes without implementations should be rendered as though their implementations are present.  (Useful for prototyping node classes.)
 * @param enableCaching Sets whether item renderings should be cached.
 *
 * @threadMain
 */
VuoRendererComposition::VuoRendererComposition(VuoComposition *baseComposition, bool renderMissingAsPresent, bool enableCaching)
	: VuoBaseDetail<VuoComposition>("VuoRendererComposition", baseComposition)
{
	getBase()->setRenderer(this);

	VuoRendererFonts::getSharedFonts();  // Load the fonts now to avoid a delay later when rendering the first item in the composition.

	setBackgroundTransparent(false);
	this->renderMissingAsPresent = renderMissingAsPresent;
	this->renderNodeActivity = false;
	this->renderPortActivity = false;
	this->renderHiddenCables = false;
	this->cachingEnabled = enableCaching;
	this->publishedInputNode = createPublishedInputNode();
	this->publishedOutputNode = createPublishedOutputNode();

	parser = NULL;

	signaler = new VuoRendererSignaler();

	addComponentsInCompositionToCanvas();
}

/**
 * Add any nodes, cables, published ports, and published cables that are already in the base composition.
 */
void VuoRendererComposition::addComponentsInCompositionToCanvas()
{
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

	foreach (VuoComment *comment, getBase()->getComments())
		addCommentInCompositionToCanvas(comment);

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
void VuoRendererComposition::addNode(VuoNode *n, bool nodeShouldBeRendered, bool nodeShouldBeGivenUniqueIdentifier)
{
	if (getBase()->hasCompiler() && nodeShouldBeGivenUniqueIdentifier)
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
	addItem(rc);

	// The following VuoRendererCable::setFrom()/setTo() calls are unnecessary as far as the
	// base cable is concerned, but forces the connected component renderings to update appropriately.
	rc->setFrom(rc->getBase()->getFromNode(), rc->getBase()->getFromPort());
	rc->setTo(rc->getBase()->getToNode(), rc->getBase()->getToPort());

	// Performance optimizations
	rc->setCacheMode(getCurrentDefaultCacheMode());
}

/**
 * Creates a renderer detail for the base comment.
 */
VuoRendererComment * VuoRendererComposition::createRendererComment(VuoComment *baseComment)
{
	return new VuoRendererComment(baseComment, signaler);
}

/**
 * Adds a comment to the canvas and the underlying composition.
 *
 * If the comment doesn't have a renderer detail, one is created for it.
 */
void VuoRendererComposition::addComment(VuoComment *c)
{
	if (getBase()->hasCompiler())
		getBase()->getCompiler()->setUniqueGraphvizIdentifierForComment(c);

	getBase()->addComment(c);
	addCommentInCompositionToCanvas(c);
}

/**
 * Adds a comment to the canvas. Assumes the comment is already in the underlying composition.
 *
 * If the comment doesn't have a renderer detail, one is created for it.
 */
void VuoRendererComposition::addCommentInCompositionToCanvas(VuoComment *c)
{
	VuoRendererComment *rc = (c->hasRenderer() ? c->getRenderer() : createRendererComment(c));
	addItem(rc);

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
 * Removes a comment from the canvas and the underlying composition.
 */
void VuoRendererComposition::removeComment(VuoRendererComment *rc)
{
	rc->updateGeometry();
	removeItem(rc);

	getBase()->removeComment(rc->getBase());
}

/**
 * Creates and connects the appropriate input attachments to the provided @c node.
 */
QList<QGraphicsItem *> VuoRendererComposition::createAndConnectInputAttachments(VuoRendererNode *node, VuoCompiler *compiler, bool createButDoNotAdd)
{
	QList<QGraphicsItem *> componentsAttached;
	if (VuoStringUtilities::beginsWith(node->getBase()->getNodeClass()->getClassName(), "vuo.math.calculate"))
	{
		VuoPort *valuesPort = node->getBase()->getInputPortWithName("values");
		set<VuoRendererInputAttachment *> attachments = valuesPort->getRenderer()->getAllUnderlyingUpstreamInputAttachments();
		if (attachments.empty())
			componentsAttached.append(createAndConnectDrawersToReadOnlyDictionaryInputPorts(node, compiler, createButDoNotAdd));
	}

	componentsAttached.append(createAndConnectDrawersToListInputPorts(node, compiler, createButDoNotAdd));

	return componentsAttached;
}

/**
 * Creates and connects a "Make List" drawer to each of the provided node's list input ports.
 */
QList<QGraphicsItem *> VuoRendererComposition::createAndConnectDrawersToListInputPorts(VuoRendererNode *node, VuoCompiler *compiler, bool createButDoNotAdd)
{
	QList<QGraphicsItem *> componentsAttached;
	foreach (VuoPort *port, node->getBase()->getInputPorts())
	{
		VuoCompilerInputEventPort *inputEventPort = (port->hasCompiler()? dynamic_cast<VuoCompilerInputEventPort *>(port->getCompiler()) : NULL);
		if (inputEventPort && VuoCompilerType::isListType(inputEventPort->getDataType()))
		{
			if (port->getRenderer()->getAllUnderlyingUpstreamInputAttachments().empty())
			{
				VuoRendererCable *cable = NULL;
				VuoRendererNode *makeListNode = createAndConnectMakeListNode(node->getBase(), port, compiler, cable);

				if (cable)
					componentsAttached.append(cable);

				if (makeListNode)
					componentsAttached.append(makeListNode);

				if (!createButDoNotAdd)
				{
					addNode(makeListNode->getBase());
					addCable(cable->getBase());
				}
			}
		}
	}

	return componentsAttached;
}

/**
 * Creates and connects the appropriate read-only dictionary attachments to the provided @c node.
 */
QList<QGraphicsItem *> VuoRendererComposition::createAndConnectDrawersToReadOnlyDictionaryInputPorts(VuoRendererNode *node, VuoCompiler *compiler, bool createButDoNotAdd)
{
	set<VuoRendererNode *> nodesToAdd;
	set<VuoRendererCable *> cablesToAdd;
	createAndConnectDictionaryAttachmentsForNode(node->getBase(), compiler, nodesToAdd, cablesToAdd);

	QList<QGraphicsItem *> componentsAttached;
	foreach (VuoRendererNode *node, nodesToAdd)
		componentsAttached.append(node);
	foreach (VuoRendererCable *cable, cablesToAdd)
		componentsAttached.append(cable);

	if (!createButDoNotAdd)
	{
		foreach (VuoRendererNode *node, nodesToAdd)
			addNode(node->getBase());

		foreach (VuoRendererCable *cable, cablesToAdd)
			addCable(cable->getBase());
	}

	return componentsAttached;
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
	vector<string> inputVariables = extractInputVariableListFromExpressionsConstant(expressionConstant, node->getNodeClass()->getClassName());
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
 *
 * Excludes variables that shouldn't be presented in the UI ("x" and "i" in the `Calculate List` node).
 */
vector<string> VuoRendererComposition::extractInputVariableListFromExpressionsConstant(string constant, string nodeClassName)
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
				{
					const char *variableName = json_object_get_string(itemObject);
					if (nodeClassName == "vuo.math.calculate.list"
							&& (strcasecmp(variableName, "x") == 0
							 || strcasecmp(variableName, "i") == 0))
						continue;

					inputVariables.push_back(json_object_get_string(itemObject));
				}
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
	publishedPort->setName(getUniquePublishedPortName(name));
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
 * Returns a string derived from the input @c baseName that is guaranteed to be unique
 * among the published input and output port names for this composition.
 */
string VuoRendererComposition::getUniquePublishedPortName(string baseName)
{
	auto isNameAvailable = [this] (const string &name)
	{
		return (! name.empty() &&
				name != "refresh" &&
				getBase()->getPublishedInputPortWithName(name) == nullptr &&
				getBase()->getPublishedOutputPortWithName(name) == nullptr);
	};

	return VuoStringUtilities::formUniqueIdentifier(isNameAvailable, baseName);
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
	// Don't try to collapse nodes that don't qualify as typecasts.
	if (!rn || !rn->getBase()->isTypecastNode())
		return NULL;

	if ((rn->getBase()->getInputPorts().size() < VuoNodeClass::unreservedInputPortStartIndex+1) ||
			(rn->getBase()->getOutputPorts().size() < VuoNodeClass::unreservedOutputPortStartIndex+1))
		return NULL;

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

	// The typecast port may have been added to a drawer,
	// and may thus need to change the horizontal position of its name.
	tp->updateNameRect();

	toRN->layoutConnectedInputDrawersAtAndAbovePort(tp);

	return tp;
}

/**
 * Convert all collapsed typecast mini-nodes in the composition back into freestanding form.
 */
void VuoRendererComposition::uncollapseTypecastNodes()
{
	foreach (VuoNode *node, getBase()->getNodes())
	{
		if (node->isTypecastNode())
		{
			collapseTypecastNode(node->getRenderer());
			uncollapseTypecastNode(node->getRenderer());
		}
	}
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
 * Removes connection eligibility highlighting from all ports and cables in the scene.
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
			(*inputPort)->getRenderer()->setEligibilityHighlight(VuoRendererColors::noHighlight);

			(*inputPort)->getRenderer()->setCacheMode(normalCacheMode);

			VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>((*inputPort)->getRenderer());
			if (typecastPort)
			{
				QGraphicsItem::CacheMode normalCacheMode = typecastPort->getChildPort()->cacheMode();
				typecastPort->getChildPort()->setCacheMode(QGraphicsItem::NoCache);

				typecastPort->getChildPort()->updateGeometry();
				typecastPort->getChildPort()->setEligibilityHighlight(VuoRendererColors::noHighlight);

				typecastPort->getChildPort()->setCacheMode(normalCacheMode);
			}
		}

		vector<VuoPort *> outputPorts = node->getOutputPorts();
		for(vector<VuoPort *>::iterator outputPort = outputPorts.begin(); outputPort != outputPorts.end(); ++outputPort)
		{
			QGraphicsItem::CacheMode normalCacheMode = (*outputPort)->getRenderer()->cacheMode();
			(*outputPort)->getRenderer()->setCacheMode(QGraphicsItem::NoCache);

			(*outputPort)->getRenderer()->updateGeometry();
			(*outputPort)->getRenderer()->setEligibilityHighlight(VuoRendererColors::noHighlight);

			(*outputPort)->getRenderer()->setCacheMode(normalCacheMode);
		}


		VuoRendererNode *rn = node->getRenderer();
		QGraphicsItem::CacheMode normalCacheMode = rn->cacheMode();
		rn->setCacheMode(QGraphicsItem::NoCache);

		rn->updateGeometry();
		rn->setEligibilityHighlight(VuoRendererColors::noHighlight);

		rn->setCacheMode(normalCacheMode);
	}

	foreach (VuoCable *cable, getBase()->getCables())
	{
		VuoRendererCable *rc = cable->getRenderer();
		QGraphicsItem::CacheMode normalCacheMode = rc->cacheMode();
		rc->setCacheMode(QGraphicsItem::NoCache);
		rc->updateGeometry();

		rc->setEligibilityHighlight(VuoRendererColors::noHighlight);

		rc->setCacheMode(normalCacheMode);
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
 * search spotlighting) by nodes within this composition should be reflected in
 * the rendering of the composition.
 */
bool VuoRendererComposition::getRenderNodeActivity(void)
{
	return this->renderNodeActivity;
}

/**
 * Returns the boolean indicating whether recent activity (e.g., trigger port
 * executions) by ports within this composition should be reflected in
 * the rendering of the composition.
 */
bool VuoRendererComposition::getRenderPortActivity(void)
{
	return (this->renderNodeActivity && this->renderPortActivity);
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

	if (VuoRendererItem::shouldDrawBoundingRects())
	{
		painter->setRenderHint(QPainter::Antialiasing, true);
		painter->setPen(QPen(QColor(255,0,0,128),5));
		painter->drawRect(sceneRect());
		QVector<QPointF> points;
		points << QPointF(0,0);
		points << sceneRect().topLeft();
		points << sceneRect().center();
		points << sceneRect().bottomRight();
		foreach (QPointF p, points)
		{
			painter->setPen(QPen(QColor(255,0,0,128),5));
			painter->drawEllipse(p, 5, 5);
			painter->setPen(QColor(255,0,0,128));
			painter->drawText(p + QPointF(5,15) - (p == sceneRect().bottomRight() ? QPointF(70,20) : QPointF(0,0)), QString("(%1,%2)").arg(p.x()).arg(p.y()));
		}
		painter->setRenderHint(QPainter::Antialiasing, false);
	}

	// Draw grid.
	if (gridType != NoGrid)
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

		if (gridType == LineGrid)
		{
			QVector<QLineF> gridLines;
			for (qreal x = leftmostGridLine; x < rect.right(); x += gridSpacing)
				gridLines.append(QLineF(x + nodeXAlignmentCorrection, rect.top(), x + nodeXAlignmentCorrection, rect.bottom()));
			for (qreal y = topmostGridLine; y < rect.bottom(); y += gridSpacing)
				gridLines.append(QLineF(rect.left(), y, rect.right(), y));

			painter->setPen(QColor(128, 128, 128, VuoRendererComposition::gridOpacity * 32));
			painter->drawLines(gridLines);
		}
		else if (gridType == PointGrid)
		{
			painter->setRenderHint(QPainter::Antialiasing, true);
			painter->setPen(Qt::NoPen);
			painter->setBrush(QColor(128, 128, 128, VuoRendererComposition::gridOpacity * 128));

			for (qreal y = topmostGridLine; y < rect.bottom(); y += gridSpacing)
				for (qreal x = leftmostGridLine; x < rect.right(); x += gridSpacing)
					// Offset by a half-pixel to render a softer plus-like point (instead of a sharp pixel-aligned square).
					painter->drawEllipse(QPointF(x + nodeXAlignmentCorrection + .5, y + .5), 1.5,1.5);
		}
	}
}

/**
 * Sets the boolean indicating whether recent activity by components within
 * this composition should be reflected in the rendering of the composition;
 * if toggling from 'false' to 'true', resets the time of last activity
 * for each component.
 */
void VuoRendererComposition::setRenderActivity(bool render, bool includePortActivity)
{
	if ((this->renderNodeActivity == render) && (this->renderPortActivity == includePortActivity))
		return;

	this->renderNodeActivity = render;
	this->renderPortActivity = includePortActivity;

	if (render)
	{
		foreach (VuoNode *node, getBase()->getNodes())
		{
			node->getRenderer()->resetTimeLastExecuted();

			if (includePortActivity)
			{
				foreach (VuoPort *port, node->getInputPorts())
					port->getRenderer()->resetTimeLastEventFired();

				foreach (VuoPort *port, node->getOutputPorts())
					port->getRenderer()->resetTimeLastEventFired();
			}
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
	this->renderHiddenCables = render;
	updateGeometryForAllComponents();
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
	return ((cachingEnabled && !renderNodeActivity)? QGraphicsItem::DeviceCoordinateCache : QGraphicsItem::NoCache);
}

/**
 * As a workaround for a bug in Qt 5.1.0-beta1 (https://b33p.net/kosada/node/4905),
 * this function must be called to create the NSAutoreleasePool for a QApplication.
 */
void VuoRendererComposition::createAutoreleasePool(void)
{
#ifdef __APPLE__
	// [NSAutoreleasePool new];
	Class poolClass = objc_getClass("NSAutoreleasePool");
	((void (*)(id, SEL))objc_msgSend)((id)poolClass, sel_getUid("new"));
#endif
}

/**
 * Maps all relative resource paths referenced within this composition's port constants
 * to the corresponding paths appropriate for use within the "Resources" directory of an
 * exported bundle.
 */
void VuoRendererComposition::modifyAllResourcePathsForBundle(void)
{
	foreach (VuoNode *node, getBase()->getNodes())
	{
		foreach (VuoPort *port, node->getInputPorts())
		{
			if (!port->hasRenderer())
				continue;

			VuoRendererPort *rp = port->getRenderer();
			if (rp->hasRelativeReadURLConstantValue())
			{
				VuoUrl url = VuoMakeRetainedFromString(rp->getConstantAsString().c_str(), VuoUrl);
				string modifiedRelativeResourcePath = modifyResourcePathForBundle(url);
				VuoRelease(url);
				rp->setConstant("\"" + modifiedRelativeResourcePath + "\"");
			}
		}
	}
}

/**
 * Maps all relative resource paths referenced within this composition's port constants
 * to the corresponding paths appropriate for use after the composition has been
 * relocated to @c newDir.
 */
map<VuoPort *, string> VuoRendererComposition::getPortConstantResourcePathsRelativeToDir(QDir newDir)
{
	map<VuoPort *, string> modifiedPathForPort;
	foreach (VuoNode *node, getBase()->getNodes())
	{
		foreach (VuoPort *port, node->getInputPorts())
		{
			if (!port->hasRenderer())
				continue;

			VuoRendererPort *rp = port->getRenderer();
			if (rp->hasRelativeReadURLConstantValue())
			{
				VuoUrl url = VuoMakeRetainedFromString(rp->getConstantAsString().c_str(), VuoUrl);
				QString origRelativeResourcePath = url;
				VuoRelease(url);
				string modifiedRelativeResourcePath = modifyResourcePathForNewDir(origRelativeResourcePath.toUtf8().constData(), newDir);

				if (modifiedRelativeResourcePath != origRelativeResourcePath.toUtf8().constData())
					modifiedPathForPort[port] = "\"" + modifiedRelativeResourcePath + "\"";
			}
		}
	}

	return modifiedPathForPort;
}

/**
 * Maps the relative path of the composition's app icon to the corresponding paths appropriate for use
 * after the composition has been relocated to @c newDir.
 */
string VuoRendererComposition::getAppIconResourcePathRelativeToDir(QDir newDir)
{
	string iconURL = getBase()->getMetadata()->getIconURL();
	string quotedIconURL = "\"" + iconURL + "\"";

	VuoUrl url = VuoMakeRetainedFromString(quotedIconURL.c_str(), VuoUrl);
	bool isRelativePath = VuoUrl_isRelativePath(url);
	QString origRelativeResourcePath = url;
	VuoRelease(url);

	if (!isRelativePath)
		return iconURL;

	return modifyResourcePathForNewDir(origRelativeResourcePath.toUtf8().constData(), newDir);
}

/**
 * Copies resources referenced within the composition by relative URL into the
 * provided @c targetResourceDir.
 *
 * If `tmpFilesOnly` is `true`, bundles only files that are originally located
 * within the `/tmp` directory or one of its subdirectories and ignores all others.
 *
 * If a non-empty `bundledIconPath` is provided, that target path will be used for the app icon,
 * instead of an automatically determined path.
 */
void VuoRendererComposition::bundleResourceFiles(string targetResourceDir, bool tmpFilesOnly, QString bundledIconPath)
{
	// Update relative URLs referenced within port constants.
	foreach (VuoNode *node, getBase()->getNodes())
	{
		foreach (VuoPort *port, node->getInputPorts())
		{
			if (!port->hasRenderer())
				continue;

			VuoRendererPort *rp = port->getRenderer();
			if (rp->hasRelativeReadURLConstantValue())
			{
				VuoUrl url = VuoMakeRetainedFromString(rp->getConstantAsString().c_str(), VuoUrl);
				QString origRelativeResourcePath = url;
				QString modifiedRelativeResourcePath = modifyResourcePathForBundle(origRelativeResourcePath.toUtf8().constData()).c_str();

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

					VDebugLog("Copying \"%s\" (from %s:%s)", url, node->getTitle().c_str(), port->getClass()->getName().c_str());
					VuoFileUtilities::copyDirectory(sourceFilePath.toStdString(), targetFilePath.toStdString());

					if (VuoFileType_isFileOfType(sourceFilePath.toUtf8().constData(), VuoFileType_Scene))
						bundleAuxiliaryFilesForSceneFile(sourceFilePath, targetFilePath);
				}
				VuoRelease(url);
			}

		}
	}

	// Update relative path to custom icon.
	string iconURL = getBase()->getMetadata()->getIconURL();
	string quotedIconURL = "\"" + iconURL + "\"";

	VuoUrl url = VuoMakeRetainedFromString(quotedIconURL.c_str(), VuoUrl);
	bool iconHasRelativePath = VuoUrl_isRelativePath(url);
	QString origRelativeResourcePath = url;

	if (iconHasRelativePath)
	{
		QString modifiedRelativeResourcePath = modifyResourcePathForBundle(origRelativeResourcePath.toUtf8().constData()).c_str();

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

			QString targetFilePath = (!bundledIconPath.isEmpty()? bundledIconPath :
																  appDir.filePath(QDir(modifiedRelativeDir.c_str()).filePath(resourceFileName.c_str())));

			// Case: Icon is already in .icns format; copy it directly.
			if (QString(ext.c_str()).toLower() == "icns")
			{
				VDebugLog("Copying \"%s\" (app icon)", url);
				VuoFileUtilities::copyDirectory(sourceFilePath.toStdString(), targetFilePath.toStdString());
			}
			// Case: Icon is in some other format; convert it to an ".icns" file.
			else
			{
				VDebugLog("Converting \"%s\" (app icon)", url);
				QPixmap icon(sourceFilePath);
				QFile file(targetFilePath);
				file.open(QIODevice::WriteOnly);
				icon.save(&file, "ICNS");
			}
		}
	}
	VuoRelease(url);

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
string VuoRendererComposition::modifyResourcePathForBundle(string path)
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
		{
			VDebugLog("Copying \"%s\"", QFileInfo(targetFile).fileName().toUtf8().constData());
			VuoFileUtilities::copyDirectory(sourceFile.toStdString(), targetFile.toStdString());
		}
	}

	// Bundle texture folders in the same directory as the mesh file.
	QStringList textureFolderNames = QStringList() << "Textures" << "_Textures";
	sourceDir.setNameFilters(textureFolderNames);
	foreach (QString textureFolderName, sourceDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
	{
		QString sourceTextureDir = QString(sourceDirName.c_str()) + QDir::separator() + textureFolderName;
		QString targetTextureDir = QString(targetDirName.c_str()) + QDir::separator() + textureFolderName;
		if (!QFileInfo(targetTextureDir).exists())
		{
			VDebugLog("Copying \"%s\"", QFileInfo(targetTextureDir).fileName().toUtf8().constData());
			VuoFileUtilities::copyDirectory(sourceTextureDir.toStdString(), targetTextureDir.toStdString());
		}
	}

	// @todo: Bundle texture folders in the parent directory of the mesh file.
	// See https://b33p.net/kosada/node/9390, https://b33p.net/kosada/node/9391.

	return;
}

/**
 * Returns a boolean indicating whether the provided @c path refers to a directory
 * (excluding OS X app bundles).
 */
bool VuoRendererComposition::isDirectory(string path)
{
	QDir dir(path.c_str());
	return dir.exists() && !VuoFileType_isFileOfType(path.c_str(), VuoFileType_App);
}

/**
 * Specifies the opacity at which grid lines/points should be rendered on the canvas.
 */
void VuoRendererComposition::setGridOpacity(int opacity)
{
	VuoRendererComposition::gridOpacity = opacity;
}

/**
 * Returns the opacity at which grid lines should be rendered on the canvas.
 */
int VuoRendererComposition::getGridOpacity()
{
	return VuoRendererComposition::gridOpacity;
}

/**
 * Specifies the type of grid to render.
 */
void VuoRendererComposition::setGridType(GridType type)
{
	VuoRendererComposition::gridType = type;
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
