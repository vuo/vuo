/**
 * @file
 * VuoProtocol interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoRunner;

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
	static string imageTransition;

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
	bool isCompositionCompliant(VuoRunner *runner);

	static vector<VuoProtocol *> getCompositionProtocols(string compositionAsString);
	static vector<VuoProtocol *> getCompositionProtocols(VuoRunner *runner);

private:
	static vector<VuoProtocol *> protocols;
	string id;
	string name;
	vector<pair<string, string> > inputPortNamesAndTypes;
	vector<pair<string, string> > outputPortNamesAndTypes;
};
