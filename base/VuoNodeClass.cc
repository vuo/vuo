/**
 * @file
 * VuoNodeClass implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoNodeClass.hh"

#include "VuoNode.hh"

#include "VuoPort.hh"
#include "VuoPortClass.hh"

#include "VuoStringUtilities.hh"

/// The starting index for unreserved input ports within the @c inputPortClasses list.
const int VuoNodeClass::unreservedInputPortStartIndex = 1;  // 0 is refresh port
/// The starting index for unreserved output ports within the @c outputPortClasses list.
const int VuoNodeClass::unreservedOutputPortStartIndex = 0;
/// The class name of the Vuo published input pseudo-node.
const string VuoNodeClass::publishedInputNodeClassName = "vuo.in";
/// The class name of the Vuo published output pseudo-node.
const string VuoNodeClass::publishedOutputNodeClassName = "vuo.out";
/// The Graphviz identifier of the Vuo published input pseudo-node.
const string VuoNodeClass::publishedInputNodeIdentifier = "PublishedInputs";
/// The Graphviz identifier of the Vuo published output pseudo-node.
const string VuoNodeClass::publishedOutputNodeIdentifier = "PublishedOutputs";

/**
 * Create a dummy base node class with no implementation.
 *
 * @param className The node class name.  See @c #getClassName.
 * @param inputPortClassNames Strings representing the names of this node class's input ports.  This list **should not** include the refresh port.
 * @param outputPortClassNames Strings representing the names of this node class's output ports.
 */
VuoNodeClass::VuoNodeClass(string className, vector<string> inputPortClassNames, vector<string> outputPortClassNames)
	: VuoBase<VuoCompilerNodeClass,void>("VuoNodeClass with string ports"),
	  VuoModule(className)
{
	this->refreshPortClass = new VuoPortClass("refresh", VuoPortClass::eventOnlyPort);
	inputPortClasses.push_back(this->refreshPortClass);
	this->interface = false;
	this->deprecated = false;

	for (vector<string>::iterator i = inputPortClassNames.begin(); i != inputPortClassNames.end(); ++i)
	{
		VuoPortClass *portClass = new VuoPortClass(*i, VuoPortClass::notAPort);
		inputPortClasses.push_back(portClass);
	}
	for (vector<string>::iterator i = outputPortClassNames.begin(); i != outputPortClassNames.end(); ++i)
	{
		VuoPortClass *portClass = new VuoPortClass(*i, VuoPortClass::notAPort);
		outputPortClasses.push_back(portClass);
	}
}

/**
 * Create a base node class with actual ports.
 *
 * @param className The node class name.  See @c #getClassName.
 * @param refreshPortClass The refresh port class.
 * @param inputPortClasses This node class's input ports.  This list **should** include @c refreshPortClass.
 * @param outputPortClasses This node class's output ports.
 */
VuoNodeClass::VuoNodeClass(string className, VuoPortClass * refreshPortClass, vector<VuoPortClass *> inputPortClasses, vector<VuoPortClass *> outputPortClasses)
	: VuoBase<VuoCompilerNodeClass,void>("VuoNodeClass with actual ports"),
	  VuoModule(className)
{
	this->refreshPortClass = refreshPortClass;
	this->inputPortClasses = inputPortClasses;
	this->outputPortClasses = outputPortClasses;
}

/**
 * Destructor.
 */
VuoNodeClass::~VuoNodeClass(void)
{
	for (vector<VuoPortClass *>::iterator i = inputPortClasses.begin(); i != inputPortClasses.end(); ++i)
		delete *i;

	for (vector<VuoPortClass *>::iterator i = outputPortClasses.begin(); i != outputPortClasses.end(); ++i)
		delete *i;
}

/**
 * Creates a dummy base node instance from this node class.
 *
 * The node by this method does not include a VuoCompilerNode.  If you want to create a substantial node instance, use @c VuoCompilerNodeClass::newNode instead.
 */
VuoNode * VuoNodeClass::newNode(string title, double x, double y)
{
	string nodeTitle = title.empty() ? getDefaultTitle() : title;

	vector<VuoPort *> inputPorts;
	VuoPort * refreshPort = NULL;
	for (vector<VuoPortClass *>::iterator it = inputPortClasses.begin(); it != inputPortClasses.end(); ++it)
	{
		VuoPort * p = new VuoPort(*it);
		inputPorts.push_back(p);
		if (*it == refreshPortClass)
			refreshPort = p;
	}

	vector<VuoPort *> outputPorts;
	for (vector<VuoPortClass *>::iterator it = outputPortClasses.begin(); it != outputPortClasses.end(); ++it)
	{
		VuoPort * p = new VuoPort(*it);
		outputPorts.push_back(p);
	}

	return new VuoNode(this, nodeTitle, refreshPort, inputPorts, outputPorts, x, y);
}

/**
 * Creates a dummy base node instance with its metadata copied from the existing @c nodeToCopyMetadataFrom.
 *
 * The node returned by this method does not include a VuoCompilerNode.  If you want to create a substantial node instance, use @c VuoCompilerNodeClass::newNode instead.
 */
VuoNode * VuoNodeClass::newNode(VuoNode *nodeToCopyMetadataFrom)
{
	VuoNode *clonedNode = newNode();

	clonedNode->setTitle(nodeToCopyMetadataFrom->getTitle());
	clonedNode->setX(nodeToCopyMetadataFrom->getX());
	clonedNode->setY(nodeToCopyMetadataFrom->getY());
	clonedNode->setTintColor(nodeToCopyMetadataFrom->getTintColor());

	return clonedNode;
}

/**
 * The unique class name for this node class.  Matches the @c .vuonode filename, minus the extension.
 *
 * Possible characters: @c [A-Za-z0-9.]
 *
 * @eg{vuo.math.lessThan.i64}
 */
string VuoNodeClass::getClassName(void)
{
	return getModuleKey();
}

/**
 * Returns true if this node class is a typecast node class.
 *
 * A typecast node class is a node that has exactly 1 data+event input port and 1 data+event or event-only output port.
 */
bool VuoNodeClass::isTypecastNodeClass(void)
{
	/// @todo Temporary workaround. Instead use VuoNode::isCollapsed() elsewhere. (https://b33p.net/kosada/node/5477)
	string nodeClassName = getClassName();
	return (VuoStringUtilities::beginsWith(nodeClassName, "vuo.type")
			|| VuoStringUtilities::beginsWith(nodeClassName, "vuo.data.summarize")
			|| VuoStringUtilities::beginsWith(nodeClassName, "vuo.math.round")
			|| VuoStringUtilities::beginsWith(nodeClassName, "vuo.list.count.")
			|| VuoStringUtilities::beginsWith(nodeClassName, "vuo.list.get.first.")
			|| VuoStringUtilities::beginsWith(nodeClassName, "vuo.list.get.last.")
			|| VuoStringUtilities::beginsWith(nodeClassName, "vuo.list.populated.")
			|| VuoStringUtilities::beginsWith(nodeClassName, "vuo.list.get.random.")
			|| VuoStringUtilities::beginsWith(nodeClassName, "vuo.scene.frameRequest.get")
			|| VuoStringUtilities::beginsWith(nodeClassName, "vuo.transform.get.")
			|| nodeClassName == "vuo.audio.mix"
			|| nodeClassName == "vuo.audio.populated"
			|| nodeClassName == "vuo.image.populated"
			|| nodeClassName == "vuo.layer.populated"
			|| nodeClassName == "vuo.mesh.populated"
			|| nodeClassName == "vuo.scene.populated"
			|| nodeClassName == "vuo.text.populated"
			|| nodeClassName == "vuo.window.cursor.populated");

#if 0
	if (inputPortClasses.size() != VuoNodeClass::unreservedInputPortStartIndex+1)
		return false;
	if (inputPortClasses[VuoNodeClass::unreservedInputPortStartIndex]->getPortType() != VuoPortClass::dataAndEventPort)
		return false;

	if (outputPortClasses.size() != VuoNodeClass::unreservedOutputPortStartIndex+1)
		return false;
	VuoPortClass::PortType outputPortType = outputPortClasses[VuoNodeClass::unreservedOutputPortStartIndex]->getPortType();
	if ( !(outputPortType == VuoPortClass::dataAndEventPort || outputPortType == VuoPortClass::eventOnlyPort) )
		return false;

	return true;
#endif
}

/**
 * Returns true if this node class is a 'Make List' or 'Make Dictionary' node class.
 */
bool VuoNodeClass::isDrawerNodeClass(void)
{
	string nodeClassName = getClassName();
	return (VuoStringUtilities::beginsWith(nodeClassName, "vuo.list.make.") ||
			VuoStringUtilities::beginsWith(nodeClassName, "vuo.dictionary.make."));
}

/**
 * If true, this node class sends data to or receives data from somewhere external to the composition
 * (e.g., input device, file, network).
 *
 * @see VuoModuleMetadata
 */
bool VuoNodeClass::isInterface(void)
{
	return interface;
}

/**
 * Sets whether this node class is an interface.
 *
 * @see VuoModuleMetadata
 */
void VuoNodeClass::setInterface(bool isInterface)
{
	this->interface = isInterface;
}

/**
 * Returns a list of example compositions that demonstrate this node class.
 *
 * @see VuoModuleMetadata
 */
vector<string> VuoNodeClass::getExampleCompositionFileNames(void)
{
	return exampleCompositionFileNames;
}

/**
 * Sets the list of example compositions that demonstrate this node class.
 *
 * The example compositions and the node class must be packaged together in a node set.
 *
 * @see VuoModuleMetadata
 */
void VuoNodeClass::setExampleCompositionFileNames(vector<string> exampleCompositionFileNames)
{
	this->exampleCompositionFileNames = exampleCompositionFileNames;
}

/**
 * Returns a boolean indicating whether this node is deprecated.
 *
 * @see VuoModuleMetadata
 */
bool VuoNodeClass::getDeprecated(void)
{
	return deprecated;
}

/**
 * Sets the boolean indicating whether this node is deprecated.
 *
 * @see VuoModuleMetadata
 */
void VuoNodeClass::setDeprecated(bool deprecated)
{
	this->deprecated = deprecated;
}

/**
 * Returns the node class's refresh port class.
 */
VuoPortClass * VuoNodeClass::getRefreshPortClass(void)
{
	return refreshPortClass;
}

/**
 * Sets the node class's refresh port class.
 */
void VuoNodeClass::setRefreshPortClass(VuoPortClass * refreshPortClass)
{
	this->refreshPortClass = refreshPortClass;
}

/**
 * Returns a list of the node class's input port classes.
 *
 * The port classes are listed in the order defined in the node class implementation's event function,
 * with the exception that the refresh port is always present (even if not specified in the node class
 * implementation's event function) and is always first.
 */
vector<VuoPortClass *> VuoNodeClass::getInputPortClasses(void)
{
	return inputPortClasses;
}

/**
 * Sets the node class's list of input port classes.
 */
void VuoNodeClass::setInputPortClasses(vector<VuoPortClass *> inputPortClasses)
{
	this->inputPortClasses = inputPortClasses;
}

/**
 * Returns a list of the node class's output port classes.
 *
 * The port classes are listed in the order defined in the node class implementation's event function.
 */
vector<VuoPortClass *> VuoNodeClass::getOutputPortClasses(void)
{
	return outputPortClasses;
}

/**
 * Sets the node class's list of output port classes.
 */
void VuoNodeClass::setOutputPortClasses(vector<VuoPortClass *> outputPortClasses)
{
	this->outputPortClasses = outputPortClasses;
}

/**
 * Prints info about this node class and its ports, for debugging.
 */
void VuoNodeClass::print(void)
{
	printf("VuoNodeClass(%p,\"%s\")",this,getModuleKey().c_str());
	if (hasCompiler())
		printf(" VuoCompilerNodeClass(%p)",getCompiler());
	if (hasRenderer())
		printf(" VuoRendererNodeClass(%p)",getRenderer());
	printf("\n");

	for (vector<VuoPortClass *>::iterator it = inputPortClasses.begin(); it != inputPortClasses.end(); ++it)
	{
		printf("\tinput ");
		(*it)->print();
	}

	for (vector<VuoPortClass *>::iterator it = outputPortClasses.begin(); it != outputPortClasses.end(); ++it)
	{
		printf("\toutput ");
		(*it)->print();
	}

	fflush(stdout);
}
