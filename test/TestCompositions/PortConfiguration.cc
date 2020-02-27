/**
 * @file
 * PortConfiguration implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <fstream>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Winvalid-constexpr"
#include <QtTest/QtTest>
#pragma clang diagnostic pop

#include <sstream>
#include "PortConfiguration.hh"
#include "TestCompositionExecution.hh"

/**
 * Creates a PortConfiguration.
 */
PortConfiguration::PortConfiguration(string firingPortName, map<string, string> valueForInputPortName, map<string, string> valueForOutputPortName)
{
	this->itemIndex = 0;
	this->firingPortName = firingPortName;
	this->valueForInputPortName = valueForInputPortName;
	this->valueForOutputPortName = valueForOutputPortName;
}

/**
 * Returns a string representation of this PortConfiguration.
 */
string PortConfiguration::toString(void)
{
	if (! itemName.empty())
		return itemName;

	ostringstream oss;
	oss << itemIndex;

	string s;
	s += oss.str() + ": ";
	s += "firingPort=" + firingPortName + ", ";
	s += "inputPortValues={ ";
	for (map<string, string>::iterator i = valueForInputPortName.begin(); i != valueForInputPortName.end(); ++i)
		s += i->first + "=" + i->second + ", ";
	s += "}, ";
	s += "outputPortValues={ ";
	for (map<string, string>::iterator i = valueForOutputPortName.begin(); i != valueForOutputPortName.end(); ++i)
		s += i->first + "=" + i->second + ", ";
	s += "}";
	return s;
}

/**
 * Uses this PortConfiguration to set published input port values and fire an event in the running composition.
 */
void PortConfiguration::setInputValuesAndFireEvent(VuoRunner *runner)
{
	map<VuoRunner::Port *, json_object *> m;
	for (map<string, string>::iterator i = valueForInputPortName.begin(); i != valueForInputPortName.end(); ++i)
	{
		VuoRunner::Port *port = runner->getPublishedInputPortWithName(i->first);
		QVERIFY2(port != NULL, ("Unknown input port: " + i->first + " ( " + toString() + " )").c_str());
		string valueAsString = i->second;
		json_object *value = json_tokener_parse(valueAsString.c_str());
		m[port] = value;
	}
	runner->setPublishedInputPortValues(m);
	for (auto &kv : m)
		json_object_put(kv.second);

	if (! firingPortName.empty())
	{
		set<VuoRunner::Port *> firingPorts;
		if (firingPortName == "*")
		{
			vector<VuoRunner::Port *> allPorts = runner->getPublishedInputPorts();
			firingPorts.insert(allPorts.begin(), allPorts.end());
		}
		else
		{
			VuoRunner::Port *firingPort = runner->getPublishedInputPortWithName(firingPortName);
			QVERIFY2(firingPort != NULL, ("Unknown firing port: " + firingPortName + " ( " + toString() + " )").c_str());
			firingPorts.insert(firingPort);
		}
		runner->firePublishedInputPortEvent(firingPorts);
	}
}

/**
 * Checks a published output port value against the published output port values expected to result for this PortConfiguration.
 */
void PortConfiguration::checkOutputValue(VuoRunner *runner, VuoRunner::Port *port)
{
	json_object *actualValue = runner->getPublishedOutputPortValue(port);

	map<string, string>::iterator i = valueForOutputPortName.find(port->getName());
	QVERIFY2(i != valueForOutputPortName.end(), ("Unexpected output port: " + port->getName() + " ( " + toString() + " )").c_str());
	json_object *expectedValue = json_tokener_parse(i->second.c_str());

	TestCompositionExecution::checkEqual(toString(), port->getType(), actualValue, expectedValue);

	json_object_put(actualValue);

	valueForOutputPortName.erase(i);
}

/**
 * Returns true if all published output ports for this PortConfiguration have been checked.
 */
bool PortConfiguration::isDoneChecking(void)
{
	return valueForOutputPortName.empty();
}

/**
 * Reads a list of port-value pairs from a JSON object.
 */
void PortConfiguration::readValueForPortNameFromJSONObject(json_object *portValuesObject, map<string, string> &valueForPort)
{
	QCOMPARE(json_object_get_type(portValuesObject), json_type_object);

	json_object_object_foreach(portValuesObject, key, val)
	{
		string port = key;
		string value = json_object_to_json_string_ext(val, JSON_C_TO_STRING_PLAIN);
		valueForPort[port] = value;
	}
}

/**
 * Reads a list of PortConfiguration objects from a JSON file.
 */
void PortConfiguration::readListFromJSONFile(string path, list<PortConfiguration *> &portConfigurations)
{
	ifstream fin(path.c_str());
	string fileContents( (istreambuf_iterator<char>(fin)), (istreambuf_iterator<char>()));
	fin.close();

	enum json_tokener_error error;
	json_object *rootObject = json_tokener_parse_verbose(fileContents.c_str(), &error);
	QVERIFY2(rootObject != NULL, json_tokener_error_desc(error));

	json_object *tolerance;
	if (json_object_object_get_ex(rootObject, "tolerance", &tolerance))
		TestCompositionExecution::setTolerance(json_object_get_double(tolerance));

	json_object *portConfigurationListObject;
	QVERIFY( json_object_object_get_ex(rootObject, "portConfiguration", &portConfigurationListObject) );
	json_type type = json_object_get_type(portConfigurationListObject);
	QVERIFY2(type == json_type_array || type == json_type_object, "The portConfiguration key must contain either an array [] or object {}.");

	int numPortConfigurationObjects;
	struct lh_entry *it;
	if (type == json_type_array)
		numPortConfigurationObjects = json_object_array_length(portConfigurationListObject);
	else // json_type_object
	{
		numPortConfigurationObjects = json_object_object_length(portConfigurationListObject);
		it = json_object_get_object(portConfigurationListObject)->head;
	}

	for (int i = 0; i < numPortConfigurationObjects; ++i)
	{
		string itemName;
		json_object *portConfigurationObject;
		if (type == json_type_array)
			portConfigurationObject = json_object_array_get_idx(portConfigurationListObject, i);
		else // json_type_object
		{
			portConfigurationObject = (struct json_object*)it->v;
			itemName = (char *)it->k;
			it = it->next;
		}
		QVERIFY(portConfigurationObject != NULL);

		string firingPortName;
		json_object *firingPortObject;
		if (json_object_object_get_ex(portConfigurationObject, "firingPort", &firingPortObject))
			firingPortName = json_object_get_string(firingPortObject);

		map<string, string> valueForInputPort;
		json_object *inputPortValuesObject;
		if (json_object_object_get_ex(portConfigurationObject, "inputPortValues", &inputPortValuesObject))
			readValueForPortNameFromJSONObject(inputPortValuesObject, valueForInputPort);

		map<string, string> valueForOutputPort;
		json_object *outputPortValuesObject;
		if (json_object_object_get_ex(portConfigurationObject, "outputPortValues", &outputPortValuesObject))
			readValueForPortNameFromJSONObject(outputPortValuesObject, valueForOutputPort);

		PortConfiguration *p = new PortConfiguration(firingPortName, valueForInputPort, valueForOutputPort);
		p->itemIndex = i;
		p->itemName = itemName;
		portConfigurations.push_back(p);
	}

	QVERIFY(! portConfigurations.empty());
}

/**
 * Fails if this PortConfiguration is not the same as @c other.
 */
void PortConfiguration::checkEqual(PortConfiguration &other)
{
	QCOMPARE(QString(firingPortName.c_str()), QString(other.firingPortName.c_str()));

	map<string, string> otherValueForInputPortName = other.valueForInputPortName;
	for (map<string, string>::iterator i = valueForInputPortName.begin(); i != valueForInputPortName.end(); ++i)
	{
		map<string, string>::iterator o = otherValueForInputPortName.find(i->first);
		QVERIFY(o != otherValueForInputPortName.end());
		QCOMPARE(QString(i->second.c_str()), QString(o->second.c_str()));
		otherValueForInputPortName.erase(o);
	}
	QVERIFY2(otherValueForInputPortName.empty(), otherValueForInputPortName.begin()->first.c_str());

	map<string, string> otherValueForOutputPortName = other.valueForOutputPortName;
	for (map<string, string>::iterator i = valueForOutputPortName.begin(); i != valueForOutputPortName.end(); ++i)
	{
		map<string, string>::iterator o = otherValueForOutputPortName.find(i->first);
		QVERIFY(o != otherValueForOutputPortName.end());
		QCOMPARE(QString(i->second.c_str()), QString(o->second.c_str()));
		otherValueForOutputPortName.erase(o);
	}
	QVERIFY2(otherValueForOutputPortName.empty(), otherValueForOutputPortName.begin()->first.c_str());
}
