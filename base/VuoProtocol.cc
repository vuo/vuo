/**
 * @file
 * VuoProtocol implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoProtocol.hh"

#include "VuoStringUtilities.hh"

#include <dispatch/dispatch.h>

static dispatch_once_t VuoProtocolsCreated = 0;	///< Make sure this process only has a single instance of each protocol.

vector<VuoProtocol *> VuoProtocol::protocols;
string VuoProtocol::imageFilter = "VuoImageFilter";			///< Processes an existing image.
string VuoProtocol::imageGenerator = "VuoImageGenerator";	///< Produces a new image.

/**
 * Returns the avaiable protocols.
 */
vector<VuoProtocol *> VuoProtocol::getProtocols(void)
{
	dispatch_once(&VuoProtocolsCreated, ^{
					  VuoProtocol *imageFilterProtocol = new VuoProtocol(imageFilter, "Image Filter");
					  imageFilterProtocol->addInputPort("image", "VuoImage");
					  imageFilterProtocol->addInputPort("time", "VuoReal");
					  imageFilterProtocol->addOutputPort("outputImage", "VuoImage");
					  protocols.push_back(imageFilterProtocol);

					  VuoProtocol *imageGeneratorProtocol = new VuoProtocol(imageGenerator, "Image Generator");
					  imageGeneratorProtocol->addInputPort("width", "VuoInteger");
					  imageGeneratorProtocol->addInputPort("height", "VuoInteger");
					  imageGeneratorProtocol->addInputPort("time", "VuoReal");
					  imageGeneratorProtocol->addOutputPort("outputImage", "VuoImage");
					  protocols.push_back(imageGeneratorProtocol);
				  });

	return protocols;
}

/**
 * Returns the protocol with the specified unique identifier, or NULL if none matches.
 */
VuoProtocol *VuoProtocol::getProtocol(string id)
{
	getProtocols();

	for (vector<VuoProtocol *>::iterator it = protocols.begin(); it != protocols.end(); ++it)
		if ((*it)->getId() == id)
			return *it;

	VUserLog("Error: Couldn't find protocol '%s'.", id.c_str());

	return NULL;
}

/**
 * Creates a protocol.
 *
 * @param id The protocol's unique identifier.  E.g., `VuoImageFilter`.
 * @param protocolName The name of the protocol, for display purposes.
 */
VuoProtocol::VuoProtocol(string id, string protocolName)
{
	this->id = id;
	this->name = protocolName;
}

/**
 * Returns the protocol's unique identifier.
 */
string VuoProtocol::getId(void)
{
	return id;
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
vector<pair<string, string> > VuoProtocol::getInputPortNamesAndTypes(void)
{
	return inputPortNamesAndTypes;
}

/**
 * Returns an ordered list of the published output port names associated with this
 * protocol, along with their types.
 */
vector<pair<string, string> > VuoProtocol::getOutputPortNamesAndTypes(void)
{
	return outputPortNamesAndTypes;
}

/**
 * Returns a boolean indicating whether the protocol has an input port with the provided @c portName.
 */
bool VuoProtocol::hasInputPort(string portName)
{
	for (vector<pair<string, string> >::iterator i = inputPortNamesAndTypes.begin(); i != inputPortNamesAndTypes.end(); ++i)
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
	for (vector<pair<string, string> >::iterator i = outputPortNamesAndTypes.begin(); i != outputPortNamesAndTypes.end(); ++i)
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
string VuoProtocol::getTypeForInputPort(string portName)
{
	for (vector<pair<string, string> >::iterator i = inputPortNamesAndTypes.begin(); i != inputPortNamesAndTypes.end(); ++i)
	{
		string currentPortName = i->first;
		string currentPortType = i->second;
		if (currentPortName == portName)
			return currentPortType;
	}

	VUserLog("Error: Protocol '%s' has no input port with name '%s'.", this->name.c_str(), portName.c_str());
	return NULL;
}

/**
 * Returns the type associated with the output port that has the provided @c portName.
 */
string VuoProtocol::getTypeForOutputPort(string portName)
{
	for (vector<pair<string, string> >::iterator i = outputPortNamesAndTypes.begin(); i != outputPortNamesAndTypes.end(); ++i)
	{
		string currentPortName = i->first;
		string currentPortType = i->second;
		if (currentPortName == portName)
			return currentPortType;
	}

	VUserLog("Error: Protocol '%s' has no output port with name '%s'.", this->name.c_str(), portName.c_str());
	return NULL;
}

/**
 * Returns true if this composition complies with this protocol.
 *
 * A composition complies with a given protocol if, for each port specified
 * in the protocol, the composition contains a published port with a matching
 * name and type. The composition may have additional published ports and
 * still comply with the protocol.
 */
bool VuoProtocol::isCompositionCompliant(string compositionAsString)
{
	__block int charNum = 0;
	bool (^getNextLine)(string &line) = ^(string &line){
			line = "";
			while (true)
			{
				char c = compositionAsString[charNum++];
				if (c == '\n')
					break;
				if (c == 0)
					return false;
				line += c;
			}
			return true;
		};

	// label="PublishedInputs|<image>image\r|<time>time\r"
	string (^getLabel)(string line) = ^(string line){
			string labelToken = "label=\"";
			string::size_type labelLocation = line.find(labelToken);
			if (labelLocation == string::npos)
				return (string)"";

			labelLocation += labelToken.length();
			string label;
			while (labelLocation < line.length() && line[labelLocation] != '"')
				label += line[labelLocation++];

			return label;
		};

	// |<image>image\r
	bool (^getNextPort)(string &label, string &portID) = ^(string &label, string &portID){
			portID = "";
			string portToken = "|<";
			string::size_type portLocation = label.find(portToken);
			if (portLocation == string::npos)
				return false;

			portLocation += portToken.length();
			while (portLocation < label.length() && label[portLocation] != '>')
				portID += label[portLocation++];

			label = label.substr(portLocation);

			return true;
		};

	// _image_type="VuoImage"
	string (^getType)(string line, string portID) = ^(string line, string portID){
			string typeToken = "_" + portID + "_type=\"";
			string::size_type typeLocation = line.find(typeToken);
			if (typeLocation == string::npos)
				return (string)"";

			typeLocation += typeToken.length();
			string type;
			while (typeLocation < line.length() && line[typeLocation] != '"')
				type += line[typeLocation++];

			return type;
		};


	// portID => portType
	map<string,string> publishedInputs;
	map<string,string> publishedOutputs;
	string line;
	while (getNextLine(line))
	{
		// PublishedInputs [type="vuo.in" label="PublishedInputs|<image>image\r|<time>time\r" _image="" _image_type="VuoImage" _time="" _time_type="VuoReal"];
		// PublishedOutputs [type="vuo.out" label="PublishedOutputs|<outputImage>outputImage\l" _outputImage_type="VuoImage"];
		bool isPublishedInputs = false;
		bool isPublishedOutputs = false;
		if (VuoStringUtilities::beginsWith(line, "PublishedInputs [type=\"vuo.in\" label=\"PublishedInputs|<"))
			isPublishedInputs = true;
		else if (VuoStringUtilities::beginsWith(line, "PublishedOutputs [type=\"vuo.out\" label=\"PublishedOutputs|<"))
			isPublishedOutputs = true;

		if (!isPublishedInputs && !isPublishedOutputs)
			continue;

		string label = getLabel(line);
		if (label == "")
			return false;

		string portID;
		while (getNextPort(label, portID))
		{
			if (isPublishedInputs)
				publishedInputs[portID] = getType(line, portID);
			else
				publishedOutputs[portID] = getType(line, portID);
		}

	}

	// Check whether the composition contains all of the required input ports.
	vector<pair<string, string> > protocolInputs = getInputPortNamesAndTypes();
	for (vector<pair<string, string> >::iterator i = protocolInputs.begin(); i != protocolInputs.end(); ++i)
	{
		string protocolInputName = i->first;
		string protocolInputType = i->second;

		map<string,string>::iterator publishedInput = publishedInputs.find(protocolInputName);
		if (publishedInput == publishedInputs.end() || publishedInput->second != protocolInputType)
			return false;
	}

	// Check whether the composition contains all of the required output ports.
	vector<pair<string, string> > protocolOutputs = getOutputPortNamesAndTypes();
	for (vector<pair<string, string> >::iterator i = protocolOutputs.begin(); i != protocolOutputs.end(); ++i)
	{
		string protocolOutputName = i->first;
		string protocolOutputType = i->second;

		map<string,string>::iterator publishedOutput = publishedOutputs.find(protocolOutputName);
		if (publishedOutput == publishedOutputs.end() || publishedOutput->second != protocolOutputType)
			return false;
	}

	return true;
}

/**
 * Adds a published input port to the protocol.
 *
 * @param portName The name of the input port.
 * @param portType The type of the input port.
 */
void VuoProtocol::addInputPort(string portName, string portType)
{
	pair<string, string> portNameAndType = make_pair(portName, portType);

	for (vector<pair<string, string> >::iterator i = inputPortNamesAndTypes.begin(); i != inputPortNamesAndTypes.end(); ++i)
	{
		string currentPortName = i->first;
		if (currentPortName == portName)
		{
			VUserLog("Error: Protocol '%s' already had an input port with name '%s'.", this->name.c_str(), portName.c_str());
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
void VuoProtocol::addOutputPort(string portName, string portType)
{
	pair<string, string> portNameAndType = make_pair(portName, portType);

	for (vector<pair<string, string> >::iterator i = outputPortNamesAndTypes.begin(); i != outputPortNamesAndTypes.end(); ++i)
	{
		string currentPortName = i->first;
		if (currentPortName == portName)
		{
			VUserLog("Error: Protocol '%s' already had an output port with name '%s'.", this->name.c_str(), portName.c_str());
			return;
		}
	}

	outputPortNamesAndTypes.push_back(portNameAndType);
}
