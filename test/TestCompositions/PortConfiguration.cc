/**
 * @file
 * PortConfiguration implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <fstream>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#include <QtTest/QtTest>
#pragma clang diagnostic pop

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
void PortConfiguration::checkOutputValue(VuoRunner *runner, VuoRunner::Port *port)
{
	json_object *actualValue = runner->getPublishedOutputPortValue(port);

	map<string, string>::iterator i = valueForOutputPortName.find(port->getName());
	QVERIFY2(i != valueForOutputPortName.end(), ("Unexpected output port: " + port->getName() + " ( " + toString() + " )").c_str());
	json_object *expectedValue = json_tokener_parse(i->second.c_str());

	checkEqual(port->getType(), actualValue, expectedValue);

	json_object_put(actualValue);

	valueForOutputPortName.erase(i);
}

/**
 * Checks that the port values are equal (or approximately equal, for doubles).
 */
void PortConfiguration::checkEqual(string type, json_object *actualValue, json_object *expectedValue)
{
	enum json_type actualType = json_object_get_type(actualValue);
	enum json_type expectedType = json_object_get_type(expectedValue);

	string actualString = json_object_to_json_string_ext(actualValue, JSON_C_TO_STRING_PLAIN);
	string expectedString = json_object_to_json_string_ext(expectedValue, JSON_C_TO_STRING_PLAIN);

	string failMessage = toString() + " --- " + expectedString + " != " + actualString;

	if (expectedType == json_type_object && actualType == json_type_object)
	{
		if (type == "VuoImage")
		{
			VuoImage expectedImage = VuoImage_makeFromJson(expectedValue);
			VuoImage   actualImage = VuoImage_makeFromJson(actualValue);
			QVERIFY2(VuoImage_areEqualWithinTolerance(actualImage, expectedImage, 1), failMessage.c_str());
			return;
		}

		json_object_object_foreach(expectedValue, expectedPort, expectedElement)
		{
			json_object *actualElement;
			QVERIFY2(json_object_object_get_ex(actualValue, expectedPort, &actualElement), failMessage.c_str());
		}
		json_object_object_foreach(actualValue, actualPort, actualElement)
		{
			json_object *expectedElement;
			QVERIFY2(json_object_object_get_ex(expectedValue, actualPort, &expectedElement), failMessage.c_str());

			checkEqual("", actualElement, expectedElement);
		}
	}
	else if (expectedType == json_type_array && actualType == json_type_array)
	{
		int actualElementCount = json_object_array_length(actualValue);
		int expectedElementCount = json_object_array_length(expectedValue);
		QVERIFY2(actualElementCount == expectedElementCount, failMessage.c_str());

		for (int i = 0; i < expectedElementCount; ++i)
		{
			json_object *actualElement = json_object_array_get_idx(actualValue, i);
			json_object *expectedElement = json_object_array_get_idx(expectedValue, i);
			checkEqual("", actualElement, expectedElement);
		}
	}
	else if ((expectedType == json_type_double && (actualType == json_type_double || actualType == json_type_int)) ||
			 (actualType == json_type_double && (expectedType == json_type_double || expectedType == json_type_int)))
	{
		double actualDouble = json_object_get_double(actualValue);
		double expectedDouble = json_object_get_double(expectedValue);
		const double DELTA = 0.000001;
		if (isnan(actualDouble) && isnan(expectedDouble))
		{
			// OK, since NaN was both expected and actually received.
		}
		else
			QVERIFY2(fabs(actualDouble - expectedDouble) <= DELTA, failMessage.c_str());
	}
	else
	{
		QVERIFY2(actualString == expectedString, failMessage.c_str());
	}
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

	json_object *portConfigurationListObject;
	QVERIFY( json_object_object_get_ex(rootObject, "portConfiguration", &portConfigurationListObject) );
	QCOMPARE(json_object_get_type(portConfigurationListObject), json_type_array);

	int numPortConfigurationObjects = json_object_array_length(portConfigurationListObject);
	for (int i = 0; i < numPortConfigurationObjects; ++i)
	{
		json_object *portConfigurationObject = json_object_array_get_idx(portConfigurationListObject, i);
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
