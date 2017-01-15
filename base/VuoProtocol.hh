/**
 * @file
 * VuoProtocol interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOPROTOCOL_HH
#define VUOPROTOCOL_HH

/**
 * This class represents a protocol. A protocol consists of an ordered list of
 * published port names and their associated types.
 */
class VuoProtocol
{
public:
	static vector<VuoProtocol *> getProtocols(void);

	static VuoProtocol *getProtocol(string id);
	static string imageFilter;
	static string imageGenerator;

	VuoProtocol(string id, string protocolName);

	string getId(void);
	string getName(void);
	vector<pair<string, string> > getInputPortNamesAndTypes(void);
	vector<pair<string, string> > getOutputPortNamesAndTypes(void);
	bool hasInputPort(string portName);
	bool hasOutputPort(string portName);
	string getTypeForInputPort(string portName);
	string getTypeForOutputPort(string portName);

	void addInputPort(string portName, string portType);
	void addOutputPort(string portName, string portType);

	bool isCompositionCompliant(string compositionAsString);

private:
	static vector<VuoProtocol *> protocols;
	string id;
	string name;
	vector<pair<string, string> > inputPortNamesAndTypes;
	vector<pair<string, string> > outputPortNamesAndTypes;
};

#endif // VUOPROTOCOL_HH
