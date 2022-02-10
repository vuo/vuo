/**
 * @file
 * VuoNode implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoNode.hh"

#include "VuoNodeClass.hh"
#include "VuoPort.hh"
#include "VuoStringUtilities.hh"

#include <stdio.h>


/**
 * Creates a base node instance from a @c VuoNodeClass.
 *
 * You'll probably want to use the factories @c VuoNodeClass::newNode or @c VuoCompilerNodeClass::newNode instead, for convenience.
 */
VuoNode::VuoNode(VuoNodeClass * nodeClass, string title, VuoPort * refreshPort, vector<VuoPort *> inputPorts, vector<VuoPort *> outputPorts, double x, double y, bool collapsed, VuoNode::TintColor tintColor)
	: VuoBase<VuoCompilerNode,VuoRendererNode>("VuoNode")
{
	this->nodeClass = nodeClass;
	this->title = title;
	this->refreshPort = refreshPort;
	this->inputPorts = inputPorts;
	this->outputPorts = outputPorts;
	this->x = x;
	this->y = y;
	this->collapsed = collapsed;
	this->tintColor = tintColor;
	this->forbidden = false;
}

/**
 * Returns the node class this node is an instance of.
 */
VuoNodeClass * VuoNode::getNodeClass(void)
{
	return nodeClass;
}

/**
 * Returns the input port matching the specified @c portName, if one exists.  Otherwise null.
 */
VuoPort * VuoNode::getInputPortWithName(string portName)
{
	for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
		if (portName == (*i)->getClass()->getName())
			return *i;

	return NULL;
}

/**
 * Returns the output port matching the specified @c portName, if one exists.  Otherwise null.
 */
VuoPort * VuoNode::getOutputPortWithName(string portName)
{
	for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
		if (portName == (*i)->getClass()->getName())
			return *i;

	return NULL;
}

/**
 * Returns the refresh port.
 */
VuoPort * VuoNode::getRefreshPort(void)
{
	return refreshPort;
}

/**
 * Returns all input ports.
 */
vector<VuoPort *> VuoNode::getInputPorts(void)
{
	return inputPorts;
}

/**
 * Returns the input ports that appear before (above) `port`, not including `port`.
 *
 * If `port` is not found, returns an empty list.
 */
vector<VuoPort *> VuoNode::getInputPortsBeforePort(VuoPort *port)
{
	vector<VuoPort *> foundPorts;
	for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
		if ((*i) == port)
			return foundPorts;
		else
			foundPorts.push_back(*i);
	return vector<VuoPort *>();
}

/**
 * Returns the input ports that appear after (below) `port`, not including `port`.
 *
 * If `port` is not found, returns an empty list.
 */
vector<VuoPort *> VuoNode::getInputPortsAfterPort(VuoPort *port)
{
	vector<VuoPort *> foundPorts;
	bool found = false;
	for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
	{
		if (found)
			foundPorts.push_back(*i);
		if ((*i) == port)
			found = true;
	}
	return foundPorts;
}

/**
 * Returns all output ports.
 */
vector<VuoPort *> VuoNode::getOutputPorts(void)
{
	return outputPorts;
}

/**
 * Returns the output ports that appear before (above) `port`, not including `port`.
 *
 * If `port` is not found, returns an empty list.
 */
vector<VuoPort *> VuoNode::getOutputPortsBeforePort(VuoPort *port)
{
	vector<VuoPort *> foundPorts;
	for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
		if ((*i) == port)
			return foundPorts;
		else
			foundPorts.push_back(*i);
	return vector<VuoPort *>();
}

/**
 * Returns the output ports that appear after (below) `port`, not including `port`.
 *
 * If `port` is not found, returns an empty list.
 */
vector<VuoPort *> VuoNode::getOutputPortsAfterPort(VuoPort *port)
{
	vector<VuoPort *> foundPorts;
	bool found = false;
	for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
	{
		if (found)
			foundPorts.push_back(*i);
		if ((*i) == port)
			found = true;
	}
	return foundPorts;
}

/**
 * Returns the node instance's title.
 *
 * @see VuoNode::VuoNode
 *
 * @eg{My Awesome Sum}
 */
string VuoNode::getTitle(void)
{
	return title;
}

/**
 * Sets the node instance's title.
 *
 * @see VuoNode::VuoNode
 *
 * @eg{My Awesome Sum}
 */
void VuoNode::setTitle(string title)
{
	this->title = title;
}

/**
 * Returns true if this node is a typecast node.
 */
bool VuoNode::isTypecastNode(void)
{
	return nodeClass->isTypecastNodeClass();
}

/**
 * Returns the x coordinate at which this node instance is drawn in the editor.
 */
int VuoNode::getX(void)
{
	return x;
}

/**
 * Returns the y coordinate at which this node instance is drawn in the editor.
 */
int VuoNode::getY(void)
{
	return y;
}

/**
 * Sets the x coordinate at which this node instance is drawn in the editor.
 */
void VuoNode::setX(int x)
{
	this->x = x;
}

/**
 * Sets the y coordinate at which this node instance is drawn in the editor.
 */
void VuoNode::setY(int y)
{
	this->y = y;
}

/**
 * Returns true if this node is to be rendered collapsed against its parent (output) node.
 */
bool VuoNode::isCollapsed(void)
{
	return collapsed;
}

/**
 * Sets whether this node is to be rendered collapsed against its parent (output) node.
 */
void VuoNode::setCollapsed(bool collapsed)
{
	this->collapsed = collapsed;
}

/**
 * Returns this node's tint color.
 */
enum VuoNode::TintColor VuoNode::getTintColor(void)
{
	return tintColor;
}

/**
 * Returns this node's tint color as a Graphviz color name, or emptystring if there is no tint.
 */
string VuoNode::getTintColorGraphvizName(void)
{
	return getGraphvizNameForTint(tintColor);
}

/**
 * Sets this node's tint color.
 */
void VuoNode::setTintColor(enum VuoNode::TintColor tintColor)
{
	this->tintColor = tintColor;
}

/**
 * Returns the value from @ref VuoNode::setForbidden if any, otherwise false.
 */
bool VuoNode::isForbidden(void)
{
	return forbidden;
}

/**
 * Sets whether the caller has determined that this node class isn't allowed to be added to the intended composition.
 */
void VuoNode::setForbidden(bool forbidden)
{
	this->forbidden = forbidden;
}

/**
 * Stores the node's original declaration found when parsing it from a Graphviz-formatted composition string.
 */
void VuoNode::setRawGraphvizDeclaration(string declaration)
{
	this->rawGraphvizDeclaration = declaration;
}

/**
 * Returns the node's original declaration found when parsing it from a Graphviz-formatted composition string.
 * This is useful when a Graphviz declaration for this node is needed but this node lacks a VuoCompilerNode.
 */
string VuoNode::getRawGraphvizDeclaration(void)
{
	return rawGraphvizDeclaration;
}

/**
 * Returns the node's original identifier found when parsing it from a Graphviz-formatted composition string.
 * This is useful when a Graphviz identifier for this node is needed but this node lacks a VuoCompilerNode.
 */
string VuoNode::getRawGraphvizIdentifier(void)
{
	string identifier;
	for (int j = 0; j < rawGraphvizDeclaration.length() && VuoStringUtilities::isValidCharInIdentifier(rawGraphvizDeclaration[j]); ++j)
		identifier += rawGraphvizDeclaration[j];
	return identifier;
}

/**
 * Prints info about this node and its ports, for debugging.
 */
void VuoNode::print(void)
{
	printf("VuoNode(%p,\"%s\")",this,title.c_str());
	if (hasCompiler())
		printf(" VuoCompilerNode(%p)",getCompiler());
	if (hasRenderer())
		printf(" VuoRendererNode(%p)",getRenderer());
	printf("\n");

	printf("\tposition (%d,%d)\n", x, y);
	printf("\tcollapsed %s\n", collapsed ? "true" : "false");
	printf("\ttintColor %s\n", getTintColorGraphvizName().c_str());

	for (vector<VuoPort *>::iterator it = inputPorts.begin(); it != inputPorts.end(); ++it)
	{
		printf("\tinput ");
		(*it)->print();
	}

	for (vector<VuoPort *>::iterator it = outputPorts.begin(); it != outputPorts.end(); ++it)
	{
		printf("\toutput ");
		(*it)->print();
	}

	fflush(stdout);
}

/**
 * Returns the Graphviz identifier corresponding to the provided @c tintColor.
 */
string VuoNode::getGraphvizNameForTint(enum TintColor tintColor)
{
	switch (tintColor)
	{
		case VuoNode::TintYellow:
			return "yellow";
		case VuoNode::TintTangerine:
			return "tangerine";
		case VuoNode::TintOrange:
			return "orange";
		case VuoNode::TintMagenta:
			return "magenta";
		case VuoNode::TintViolet:
			return "violet";
		case VuoNode::TintBlue:
			return "blue";
		case VuoNode::TintCyan:
			return "cyan";
		case VuoNode::TintGreen:
			return "green";
		case VuoNode::TintLime:
			return "lime";
		default:
			return "";
	}
}

/**
 * Returns the tint color corresponding to the provided Graphviz identifier.
 */
enum VuoNode::TintColor VuoNode::getTintWithGraphvizName(string tintName)
{
	if (tintName == "yellow")
		return VuoNode::TintYellow;
	else if (tintName == "tangerine")
		return VuoNode::TintTangerine;
	else if (tintName == "orange")
		return VuoNode::TintOrange;
	else if (tintName == "magenta")
		return VuoNode::TintMagenta;
	else if (tintName == "violet")
		return VuoNode::TintViolet;
	else if (tintName == "blue")
		return VuoNode::TintBlue;
	else if (tintName == "cyan")
		return VuoNode::TintCyan;
	else if (tintName == "green")
		return VuoNode::TintGreen;
	else if (tintName == "lime")
		return VuoNode::TintLime;
	else
		return VuoNode::TintNone;
}
