/**
 * @file
 * TestCompositionExecution implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "TestCompositionExecution.hh"

#include <fcntl.h>
#include <libgen.h>
#include <malloc/malloc.h>
#include <sstream>

/**
 * Sets up a compiler, adding both regular and for-testing-only node classes to the search path.
 */
VuoCompiler * TestCompositionExecution::initCompiler(void)
{
	VuoCompiler *c = new VuoCompiler();
	c->addModuleSearchPath("node-" + QDir::current().dirName().toStdString());
	return c;
}

/**
 * Returns the directory containing the test compositions.
 */
QDir TestCompositionExecution::getCompositionDir(void)
{
	QDir compositionDir = QDir::current();
	compositionDir.cd("composition");
	return compositionDir;
}

/**
 * Returns the path of the test composition (which should include the file extension).
 */
string TestCompositionExecution::getCompositionPath(string compositionFileName)
{
	return getCompositionDir().filePath(QString(compositionFileName.c_str())).toStdString();
}

/**
 * Copies the composition at @a compositionPath to the User Modules folder, renaming it to @a nodeClassName.
 */
void TestCompositionExecution::installSubcomposition(string compositionPath, string nodeClassName, VuoCompiler *compiler)
{
	string installedSubcompositionPath = VuoFileUtilities::getUserModulesPath() + "/" + nodeClassName + ".vuo";
	VuoFileUtilities::copyFile(compositionPath, installedSubcompositionPath);
	compiler->installSubcomposition(installedSubcompositionPath);
}

/**
 * Removes the subcomposition called @a nodeClassName from the User Modules folder and uninstalls it.
 */
void TestCompositionExecution::uninstallSubcomposition(string nodeClassName, VuoCompiler *compiler)
{
	compiler->uninstallSubcomposition(nodeClassName);
	string copiedCompositionPath = VuoFileUtilities::getUserModulesPath() + "/" + nodeClassName + ".vuo";
	remove(copiedCompositionPath.c_str());
}

/**
 * Returns a string containing a composition that consists of an instance of a node class
 * with all its input and output ports published.
 */
string TestCompositionExecution::wrapNodeInComposition(VuoCompilerNodeClass *nodeClass, VuoCompiler *compiler)
{
	ostringstream oss;
	oss << "digraph G {" << endl;

	VuoNode *node = compiler->createNode(nodeClass);
	oss << node->getCompiler()->getGraphvizDeclaration() << endl;
	string nodeIdentifier = node->getCompiler()->getGraphvizIdentifier();

	oss << "PublishedInputs [type=\"vuo.in\" label=\"PublishedInputs";
	foreach (VuoPort *port, node->getInputPorts())
	{
		string portName = port->getClass()->getName();
		oss << "|<" << portName << ">" << portName << "\\r";
	}
	oss << "\"";
	foreach (VuoPort *port, node->getInputPorts())
	{
		VuoCompilerInputData *data = dynamic_cast<VuoCompilerInputEventPort *>( port->getCompiler() )->getData();
		if (data)
		{
			string initialValue = VuoStringUtilities::transcodeToGraphvizIdentifier( data->getInitialValue() );
			string portName = port->getClass()->getName();
			oss << " _" << portName << "=\"" << initialValue << "\"";
		}
	}
	oss << "];" << endl;

	oss << "PublishedOutputs [type=\"vuo.out\" label=\"PublishedOutputs";
	foreach (VuoPort *port, node->getOutputPorts())
	{
		string portName = port->getClass()->getName();
		oss << "|<" << portName << ">" << portName << "\\l";
	}
	oss << "\"];" << endl;

	foreach (VuoPort *port, node->getInputPorts())
	{
		string portName = port->getClass()->getName();
		oss << "PublishedInputs:" << portName << " -> " << nodeIdentifier << ":" << portName << ";" << endl;
	}

	foreach (VuoPort *port, node->getOutputPorts())
	{
		string portName = port->getClass()->getName();
		oss << nodeIdentifier << ":" << portName << " -> " << "PublishedOutputs:" << portName << ";" << endl;
	}

	oss << "}" << endl;

	return oss.str();
}

/**
 * Workaround to avoid false positives when checking VuoHeap for memory leaks.
 *
 * This function should be called just before stopping a composition. It gives VuoImageText a chance to clean its cache.
 */
void TestCompositionExecution::waitForImageTextCacheCleanup(void)
{
	// https://b33p.net/kosada/node/9956, https://b33p.net/kosada/node/10374

	usleep(USEC_PER_SEC * 2.1);  // A little more than 2 * VuoImageTextCache_timeout (the created images won't be old enough to purge during the first cleanup 1 second after starting).
}

/**
 * Prints the amount of memory dynamically allocated by the current process.
 */
void TestCompositionExecution::printMemoryUsage(string label)
{
	// http://www.opensource.apple.com/source/Libc/Libc-763.12/include/malloc/malloc.h
	malloc_statistics_t mzs;
	malloc_zone_statistics(NULL,&mzs);

	printf("=== Memory usage %s ===\n", label.c_str());
	printf("%lu bytes used\n", mzs.size_in_use);
	printf("%lu bytes used at high-water mark\n", mzs.max_size_in_use);
}

/**
 * Returns a string description of the specified JSON type.
 */
const char *TestCompositionExecution::getJsonTypeDescription(enum json_type type)
{
	switch (type)
	{
		case json_type_null:	return "json_type_null";
		case json_type_boolean:	return "json_type_boolean";
		case json_type_double:	return "json_type_double";
		case json_type_int:		return "json_type_int";
		case json_type_object:	return "json_type_object";
		case json_type_array:	return "json_type_array";
		case json_type_string:	return "json_type_string";
	}
}

/**
 * Checks that the port values are equal (or approximately equal, for doubles).
 */
void TestCompositionExecution::checkEqual(string itemName, string type, json_object *actualValue, json_object *expectedValue)
{
	enum json_type actualType = json_object_get_type(actualValue);
	enum json_type expectedType = json_object_get_type(expectedValue);

	string actualString = json_object_to_json_string_ext(actualValue, JSON_C_TO_STRING_PLAIN);
	string expectedString = json_object_to_json_string_ext(expectedValue, JSON_C_TO_STRING_PLAIN);

	string failMessage = "\"" + itemName + "\" --- " + expectedString + " != " + actualString;

//	VLog("type=%s expectedJson=%s actualJson=%s", type.c_str(), getJsonTypeDescription(expectedType), getJsonTypeDescription(actualType));
	if (expectedType == json_type_object && actualType == json_type_object)
	{
		if (type == "VuoImage")
		{
			VuoImage expectedImage = VuoImage_makeFromJson(expectedValue);
			VuoImage   actualImage = VuoImage_makeFromJson(actualValue);
			QVERIFY2(VuoImage_areEqualWithinTolerance(actualImage, expectedImage, 1), failMessage.c_str());
			return;
		}
		else if (type == "VuoColor")
		{
			VuoColor expected = VuoColor_makeFromJson(expectedValue);
			VuoColor   actual = VuoColor_makeFromJson(actualValue);
			QVERIFY2(VuoColor_areEqualWithinTolerance(actual, expected, 0.01), failMessage.c_str());
			return;
		}
		else if (type == "VuoWindowReference")
		{
			// Since VuoWindowReference is a pointer, tests can't expect an exact value.
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

			checkEqual(itemName, "", actualElement, expectedElement);
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
			checkEqual(itemName, "", actualElement, expectedElement);
		}
	}
	else if ((expectedType == json_type_double && (actualType == json_type_double || actualType == json_type_int)) ||
			 (actualType == json_type_double && (expectedType == json_type_double || expectedType == json_type_int)))
	{
		double actualDouble = json_object_get_double(actualValue);
		double expectedDouble = json_object_get_double(expectedValue);
		const double DELTA = 0.00001;
		if (isnan(actualDouble) && isnan(expectedDouble))
		{
			// OK, since NaN was both expected and actually received.
		}
		else
			QVERIFY2(fabs(actualDouble - expectedDouble) <= DELTA, failMessage.c_str());
	}
	else
		QVERIFY2(actualString == expectedString, failMessage.c_str());
}
