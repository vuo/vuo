/**
 * @file
 * PortConfiguration interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"

	#include <json-c/json.h>

#pragma clang diagnostic pop


#include <Vuo/Vuo.h>

/**
 * A configuration of published input port values to set, a published input port to fire an event through,
 * and the published output port values expected to result.
 */
class PortConfiguration
{
private:
	int itemIndex;          ///< The index of this test datum in the array of test data.
	string itemName;		///< The display name of this test datum.
	string firingPortName;  ///< The name of the published input port to fire an event through.
	map<string, string> valueForInputPortName;  ///< The value to set for each published input port.
	map<string, string> valueForOutputPortName;  ///< The expected value to result for each published output port.

	string toString(void);

public:
	PortConfiguration(string firingPortName, map<string, string> valueForInputPortName, map<string, string> valueForOutputPortName);
	void setInputValuesAndFireEvent(VuoRunner *runner);
	void checkOutputValue(VuoRunner *runner, VuoRunner::Port *port);
	bool isDoneChecking(void);
	static void readValueForPortNameFromJSONObject(json_object *portValuesObject, map<string, string> &valueForPort);
	static void readListFromJSONFile(string path, list<PortConfiguration *> &portConfigurations);
	void checkEqual(PortConfiguration &other);
};
