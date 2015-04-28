/**
 * @file
 * VuoProtocol implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoProtocol.hh"

/**
 * Creates a protocol.
 *
 * @param protocolName The name of the protocol, for display purposes.
 */
VuoProtocol::VuoProtocol(string protocolName)
{
	this->name = protocolName;
}

/**
 * Returns the name of the protocol.
 */
string VuoProtocol::getName(void)
{
	return name;
}

/**
 * Returns an ordered list of the published input port names associated with this
 * protocol, along with their types.
 */
vector<pair<string, VuoType *> > VuoProtocol::getInputPortNamesAndTypes(void)
{
	return inputPortNamesAndTypes;
}

/**
 * Returns an ordered list of the published output port names associated with this
 * protocol, along with their types.
 */
vector<pair<string, VuoType *> > VuoProtocol::getOutputPortNamesAndTypes(void)
{
	return outputPortNamesAndTypes;
}

/**
 * Returns a boolean indicating whether the protocol has an input port with the provided @c portName.
 */
bool VuoProtocol::hasInputPort(string portName)
{
	for (vector<pair<string, VuoType *> >::iterator i = inputPortNamesAndTypes.begin(); i != inputPortNamesAndTypes.end(); ++i)
	{
		string currentPortName = i->first;
		if (currentPortName == portName)
			return true;
	}

	return false;
}

/**
 * Returns a boolean indicating whether the protocol has an output port with the provided @c portName.
 */
bool VuoProtocol::hasOutputPort(string portName)
{
	for (vector<pair<string, VuoType *> >::iterator i = outputPortNamesAndTypes.begin(); i != outputPortNamesAndTypes.end(); ++i)
	{
		string currentPortName = i->first;
		if (currentPortName == portName)
			return true;
	}

	return false;
}

/**
 * Returns the type associated with the input port that has the provided @c portName.
 */
VuoType * VuoProtocol::getTypeForInputPort(string portName)
{
	for (vector<pair<string, VuoType *> >::iterator i = inputPortNamesAndTypes.begin(); i != inputPortNamesAndTypes.end(); ++i)
	{
		string currentPortName = i->first;
		VuoType *currentPortType = i->second;
		if (currentPortName == portName)
			return currentPortType;
	}

	fprintf(stderr, "VuoProtocol::getTypeForInputPort() Error: Protocol '%s' has no input port with name '%s'.\n", this->name.c_str(), portName.c_str());
	return NULL;
}

/**
 * Returns the type associated with the output port that has the provided @c portName.
 */
VuoType * VuoProtocol::getTypeForOutputPort(string portName)
{
	for (vector<pair<string, VuoType *> >::iterator i = outputPortNamesAndTypes.begin(); i != outputPortNamesAndTypes.end(); ++i)
	{
		string currentPortName = i->first;
		VuoType *currentPortType = i->second;
		if (currentPortName == portName)
			return currentPortType;
	}

	fprintf(stderr, "VuoProtocol::getTypeForOutputPort() Error: Protocol '%s' has no output port with name '%s'.\n", this->name.c_str(), portName.c_str());
	return NULL;
}


/**
 * Adds a published input port to the protocol.
 *
 * @param portName The name of the input port.
 * @param portType The type of the input port.
 */
void VuoProtocol::addInputPort(string portName, VuoType *portType)
{
	pair<string, VuoType *> portNameAndType = make_pair(portName, portType);

	for (vector<pair<string, VuoType *> >::iterator i = inputPortNamesAndTypes.begin(); i != inputPortNamesAndTypes.end(); ++i)
	{
		string currentPortName = i->first;
		if (currentPortName == portName)
		{
			fprintf(stderr, "VuoProtocol::addInputPort() Error: Protocol '%s' already had an input port with name '%s'.\n", this->name.c_str(), portName.c_str());
			return;
		}
	}

	inputPortNamesAndTypes.push_back(portNameAndType);
}

/**
 * Adds a published output port to the protocol.
 *
 * @param portName The name of the output port.
 * @param portType The type of the output port.
 */
void VuoProtocol::addOutputPort(string portName, VuoType *portType)
{
	pair<string, VuoType *> portNameAndType = make_pair(portName, portType);

	for (vector<pair<string, VuoType *> >::iterator i = outputPortNamesAndTypes.begin(); i != outputPortNamesAndTypes.end(); ++i)
	{
		string currentPortName = i->first;
		if (currentPortName == portName)
		{
			fprintf(stderr, "VuoProtocol::addOutputPort() Error: Protocol '%s' already had an output port with name '%s'.\n", this->name.c_str(), portName.c_str());
			return;
		}
	}

	outputPortNamesAndTypes.push_back(portNameAndType);
}
