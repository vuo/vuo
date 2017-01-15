/**
 * @file
 * VuoComposition implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoComposition.hh"
#include "VuoCable.hh"
#include "VuoNode.hh"
#include "VuoPort.hh"
#include "VuoPortClass.hh"
#include "VuoPublishedPort.hh"

/**
 * Creates an empty composition.
 */
VuoComposition::VuoComposition(void)
	: VuoBase<VuoCompilerComposition,VuoRendererComposition>("VuoComposition")
{
	this->name = "VuoComposition";
	this->directory = "";
}

/**
 * Sets the composition's display name.
 */
void VuoComposition::setName(string name)
{
	this->name = name;
}

/**
 * Returns the composition's display name.
 */
string VuoComposition::getName(void)
{
	return name;
}

/**
 * Sets the directory containing the composition's .vuo source file.
 */
void VuoComposition::setDirectory(string directory)
{
	this->directory = directory;
}

/**
 * Returns the directory containing the composition's .vuo source file.
 */
string VuoComposition::getDirectory(void)
{
	return directory;
}

/**
 * Sets the composition's description (documentation).
 */
void VuoComposition::setDescription(string description)
{
	this->description = description;
}

/**
 * Returns the composition's description (documentation).
 */
string VuoComposition::getDescription(void)
{
	return description;
}

/**
 * Sets the composition's copyright.
 */
void VuoComposition::setCopyright(string copyright)
{
	this->copyright = copyright;
}

/**
 * Returns the composition's copyright.
 */
string VuoComposition::getCopyright(void)
{
	return copyright;
}

/**
 * Adds a node to the composition.
 */
void VuoComposition::addNode(VuoNode *node)
{
	nodes.insert(node);
}

/**
 * Removes a node from the composition.
 *
 * The caller is responsible for removing any cables attached to the node.
 */
void VuoComposition::removeNode(VuoNode *node)
{
	nodes.erase(node);
}

/**
 * Returns the nodes in this composition.
 */
set<VuoNode *> VuoComposition::getNodes(void)
{
	return nodes;
}


/**
 * Adds a cable to the composition.
 *
 * The caller is responsible for adding the nodes on either end of the cable to the composition.
 */
void VuoComposition::addCable(VuoCable *cable)
{
	cables.insert(cable);
}

/**
 * Disconnects and removes a cable from the composition.
 */
void VuoComposition::removeCable(VuoCable *cable)
{
	cable->setFrom(NULL, NULL);
	cable->setTo(NULL, NULL);

	cables.erase(cable);
}

/**
 * Returns the cables in this composition.
 */
set<VuoCable *> VuoComposition::getCables(void)
{
	return cables;
}

/**
 * Adds a published port to this composition's list of published input ports at the given index.
 */
void VuoComposition::addPublishedInputPort(VuoPublishedPort *port, int index)
{
	publishedInputPorts.insert(publishedInputPorts.begin() + index, port);
}

/**
 * Adds a published port to this composition's list of published output ports at the given index.
 */
void VuoComposition::addPublishedOutputPort(VuoPublishedPort *port, int index)
{
	publishedOutputPorts.insert(publishedOutputPorts.begin() + index, port);
}

/**
 * Removes the published port at the given index from this composition's list of published input ports.
 */
void VuoComposition::removePublishedInputPort(int index)
{
	publishedInputPorts.erase(publishedInputPorts.begin() + index);
}

/**
 * Removes the published port at the given index from this composition's list of published input ports.
 */
void VuoComposition::removePublishedOutputPort(int index)
{
	publishedOutputPorts.erase(publishedOutputPorts.begin() + index);
}

/**
 * Returns the published input ports in this composition.
 */
vector<VuoPublishedPort *> VuoComposition::getPublishedInputPorts(void)
{
	return publishedInputPorts;
}

/**
 * Returns the published output ports in this composition.
 */
vector<VuoPublishedPort *> VuoComposition::getPublishedOutputPorts(void)
{
	return publishedOutputPorts;
}

/**
 * Returns the published input port with the given name, or null if none matches.
 */
VuoPublishedPort * VuoComposition::getPublishedInputPortWithName(string name)
{
	return getPublishedPortWithName(name, true);
}

/**
 * Returns the published output port with the given name, or null if none matches.
 */
VuoPublishedPort * VuoComposition::getPublishedOutputPortWithName(string name)
{
	return getPublishedPortWithName(name, false);
}

/**
 * Helper function for getPublishedInputPortWithName() and getPublishedOutputPortWithName().
 */
VuoPublishedPort * VuoComposition::getPublishedPortWithName(string name, bool isInput)
{
	vector<VuoPublishedPort *> publishedPorts = (isInput ? publishedInputPorts : publishedOutputPorts);
	for (vector<VuoPublishedPort *>::iterator i = publishedPorts.begin(); i != publishedPorts.end(); ++i)
	{
		VuoPublishedPort *port = *i;
		if (port->getClass()->getName() == name)
			return port;
	}
	return NULL;
}

/**
 * Returns the index of the published port in the list of published input or output ports.
 */
int VuoComposition::getIndexOfPublishedPort(VuoPublishedPort *port, bool isInput)
{
	vector<VuoPublishedPort *> publishedPorts = (isInput ? getPublishedInputPorts() : getPublishedOutputPorts());
	vector<VuoPublishedPort *>::iterator foundPort = find(publishedPorts.begin(), publishedPorts.end(), port);
	if (foundPort != publishedPorts.end())
		return std::distance(publishedPorts.begin(), foundPort);
	else
		return -1;
}

/**
 * Replaces @a oldNode with @a newNode in the composition, transferring all cable and published port
 * connections from @a oldNode to @a newNode.
 */
void VuoComposition::replaceNode(VuoNode *oldNode, VuoNode *newNode)
{
	removeNode(oldNode);
	addNode(newNode);

	for (set<VuoCable *>::iterator i = cables.begin(); i != cables.end(); ++i)
	{
		VuoCable *cable = *i;
		if (cable->getFromNode() == oldNode)
		{
			VuoPort *oldPort = cable->getFromPort();
			VuoPort *newPort = newNode->getOutputPortWithName( oldPort->getClass()->getName() );
			cable->setFrom(newNode, newPort);
		}
		if (cable->getToNode() == oldNode)
		{
			VuoPort *oldPort = cable->getToPort();
			VuoPort *newPort = newNode->getInputPortWithName( oldPort->getClass()->getName() );
			cable->setTo(newNode, newPort);
		}
	}
}

/**
 * Parses the composition's Doxygen header to retrieve its name, description, and copyright.
 *
 * If a Doxygen line starts `@brief `, the @c name is the remainder of that line.
 *
 * If a Doxygen line starts `@copyright `, the @c copyright is the remainder of that paragraph.
 *
 * The remaining Doxygen lines are the @c description.
 */
void VuoComposition::parseHeader(const string &compositionAsString, string &name, string &description, string &copyright)
{
	__block int charNum = 0;
	string (^getNextLine)(void) = ^{
			string line;
			while (true)
			{
				char c = compositionAsString[charNum++];
				if (c == '\n')
					break;
				if (c == 0)
					return (string)"";
				line += c;
			}
			return line;
		};

	__block bool firstLine = true;
	bool (^getNextDoxygenLine)(string &line) = ^(string &line){
			while ((line = getNextLine()) != "")
			{
				if (firstLine)
				{
					firstLine = false;
					if (line != "/**")
						return false;
					else
						continue;
				}

				if (line == "*/")
					return false;

				if (line.substr(0, 1) == "*")
				{
					if (line.length() > 1)
						line = line.substr(2);
					else
						line = "";
					return true;
				}
				if (line.substr(0, 2) == " *")
				{
					if (line.length() > 2)
						line = line.substr(3);
					else
						line = "";
					return true;
				}
				return false;
			}
			return false;
		};

	string line;
	bool firstDescriptionLine = true;
	bool inCopyright = false;
	while (getNextDoxygenLine(line))
	{
		if (line == "@file")
			continue;

		if (line.substr(0, 7) == "@brief ")
		{
			name = line.substr(7);
			continue;
		}
		if (line.substr(0, 11) == "@copyright ")
		{
			inCopyright = true;
			copyright = line.substr(11);
			continue;
		}

		if (line == "")
		{
			inCopyright = false;
			continue;
		}

		if (inCopyright)
		{
			copyright += " " + line;
			continue;
		}

		if (!firstDescriptionLine)
			description += " ";
		description += line;
		firstDescriptionLine = false;
	}
}
