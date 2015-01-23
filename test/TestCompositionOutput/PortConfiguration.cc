/**
 * @file
 * PortConfiguration implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <fstream>
#include <QTest>

#include "PortConfiguration.hh"

/**
 * Creates a PortConfiguration.
 */
PortConfiguration::PortConfiguration(string firingPortName, map<string, string> valueForInputPortName, map<string, string> valueForOutputPortName)
{
	this->firingPortName = firingPortName;
	this->valueForInputPortName = valueForInputPortName;
	this->valueForOutputPortName = valueForOutputPortName;
}

/**
 * Returns a string representation of this PortConfiguration.
 */
string PortConfiguration::toString(void)
{
	string s;
	s += "firingPortName=" + firingPortName + ", ";
	s += "valueForInputPortName={ ";
	for (map<string, string>::iterator i = valueForInputPortName.begin(); i != valueForInputPortName.end(); ++i)
		s += i->first + "=" + i->second + ", ";
	s += "}, ";
	s += "valueForOutputPortName={ ";
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
	for (map<string, string>::iterator i = valueForInputPortName.begin(); i != valueForInputPortName.end(); ++i)
	{
		VuoRunner::Port *port = runner->getPublishedInputPortWithName(i->first);
		QVERIFY2(port != NULL, ("Unknown input port: " + i->first + " ( " + toString() + " )").c_str());
		string valueAsString = i->second;
		json_object *value = json_tokener_parse(valueAsString.c_str());
		runner->setPublishedInputPortValue(port, value);
		json_object_put(value);
	}

	if (! firingPortName.empty())
	{
		VuoRunner::Port *firingPort = runner->getPublishedInputPortWithName(firingPortName);
		QVERIFY2(firingPort != NULL, ("Unknown firing port: " + firingPortName + " ( " + toString() + " )").c_str());
		runner->firePublishedInputPortEvent(firingPort);
	}
}

/**
 * Checks a published output port value against the published output port values expected to result for this PortConfiguration.
 */
void PortConfiguration::checkOutputValue(VuoRunner::Port *port, string value)
{
	map<string, string>::iterator i = valueForOutputPortName.find(port->getName());
	QVERIFY2(i != valueForOutputPortName.end(), ("Unexpected output port: " + port->getName() + " ( " + toString() + " )").c_str());
	QVERIFY2(value == i->second, ("Unexpected output port value: " + port->getName() + " = " + value + " ( " + toString() + " )").c_str());
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
void PortConfiguration::readValueForPortNameFromJSONObject(json_object *valueForPortListObject, map<string, string> &valueForPort)
{
	QCOMPARE(json_object_get_type(valueForPortListObject), json_type_array);

	int numValueForPortObjects = json_object_array_length(valueForPortListObject);
	for (int j = 0; j < numValueForPortObjects; ++j)
	{
		json_object *valueForPortObject = json_object_array_get_idx(valueForPortListObject, j);
		QVERIFY(valueForPortObject != NULL);

		json_object *portObject = json_object_object_get(valueForPortObject, "port");
		QVERIFY(portObject != NULL);
		string portName = json_object_get_string(portObject);

		json_object *valueObject = json_object_object_get(valueForPortObject, "value");
		QVERIFY(valueObject != NULL);
		string value = json_object_get_string(valueObject);

		valueForPort[portName] = value;
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

	json_object *portConfigurationListObject = json_object_object_get(rootObject, "portConfiguration");
	QVERIFY(portConfigurationListObject != NULL);
	QCOMPARE(json_object_get_type(portConfigurationListObject), json_type_array);

	int numPortConfigurationObjects = json_object_array_length(portConfigurationListObject);
	for (int i = 0; i < numPortConfigurationObjects; ++i)
	{
		json_object *portConfigurationObject = json_object_array_get_idx(portConfigurationListObject, i);
		QVERIFY(portConfigurationObject != NULL);

		json_object *firingPortObject = json_object_object_get(portConfigurationObject, "firingPortName");
		string firingPortName;
		if (firingPortObject)
			firingPortName = json_object_get_string(firingPortObject);

		json_object *valueForInputPortListObject = json_object_object_get(portConfigurationObject, "valueForInputPortName");
		map<string, string> valueForInputPort;
		if (valueForInputPortListObject)
			readValueForPortNameFromJSONObject(valueForInputPortListObject, valueForInputPort);

		json_object *valueForOutputPortListObject = json_object_object_get(portConfigurationObject, "valueForOutputPortName");
		map<string, string> valueForOutputPort;
		if (valueForOutputPortListObject)
			readValueForPortNameFromJSONObject(valueForOutputPortListObject, valueForOutputPort);

		portConfigurations.push_back( new PortConfiguration(firingPortName, valueForInputPort, valueForOutputPort) );
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

	map<string, string> otherValueForOutputPortName = other.valueForOutputPortName;
	for (map<string, string>::iterator i = valueForOutputPortName.begin(); i != valueForOutputPortName.end(); ++i)
	{
		map<string, string>::iterator o = otherValueForOutputPortName.find(i->first);
		QVERIFY(o != otherValueForOutputPortName.end());
		QCOMPARE(QString(i->second.c_str()), QString(o->second.c_str()));
		otherValueForOutputPortName.erase(o);
	}
}
