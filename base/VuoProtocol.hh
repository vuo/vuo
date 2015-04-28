/**
 * @file
 * VuoProtocol interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOPROTOCOL_HH
#define VUOPROTOCOL_HH

#include "VuoType.hh"

/**
 * This class represents a protocol. A protocol consists of an ordered list of
 * published port names and their associated types.
 */
class VuoProtocol
{
public:
	VuoProtocol(string protocolName);

	string getName(void);
	vector<pair<string, VuoType *> > getInputPortNamesAndTypes(void);
	vector<pair<string, VuoType *> > getOutputPortNamesAndTypes(void);
	bool hasInputPort(string portName);
	bool hasOutputPort(string portName);
	VuoType * getTypeForInputPort(string portName);
	VuoType * getTypeForOutputPort(string portName);
	void addInputPort(string portName, VuoType *portType);
	void addOutputPort(string portName, VuoType *portType);

private:
	string name;
	vector<pair<string, VuoType *> > inputPortNamesAndTypes;
	vector<pair<string, VuoType *> > outputPortNamesAndTypes;
};

#endif // VUOPROTOCOL_HH
