/**
 * @file
 * TestCompositionExecution implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "TestCompositionExecution.hh"

#include <fcntl.h>
#include <libgen.h>
#include <malloc/malloc.h>
#include <sstream>

/**
 * Sets up a compiler, adding both regular and for-testing-only node classes to the search path.
 *
 * @param compositionPath Typically, the path of the composition that will be compiled. If there isn't one, you can pass
 *    a unique string (e.g. the name of the test function) to assign the test a unique composition-scope module cache.
 */
VuoCompiler * TestCompositionExecution::initCompiler(const string &compositionPath)
{
	VuoCompiler *c = new VuoCompiler(compositionPath);
	c->environments.back().at(0)->addModuleSearchPath(BINARY_DIR "/test/" + QDir::current().dirName().toStdString(), false);
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
void TestCompositionExecution::installSubcomposition(string compositionPath, string nodeClassName)
{
	string installedSubcompositionPath = VuoFileUtilities::getUserModulesPath() + "/" + nodeClassName + ".vuo";
	VuoFileUtilities::copyFile(compositionPath, installedSubcompositionPath);
}

/**
 * Removes the subcomposition called @a nodeClassName from the User Modules folder.
 */
void TestCompositionExecution::uninstallSubcomposition(string nodeClassName)
{
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
 * Returns the first port in the specified list whose type matches the specified type,
 * or null if there are no ports of that type.
 */
VuoPortClass *TestCompositionExecution::getFirstPortOfType(vector<VuoPortClass *> ports, string type)
{
	for (VuoPortClass *p : ports)
	{
		VuoCompilerPortClass *cpc = dynamic_cast<VuoCompilerPortClass *>(p->getCompiler());
		VuoType *dataType = cpc->getDataVuoType();
		if (!dataType)
			continue;
		if (cpc->getDataVuoType()->getModuleKey() == type)
			return p;
	}
	return nullptr;
}

class TestCompositionExecutionDelegate : public VuoRunnerDelegateAdapter
{
	void lostContactWithComposition(void)
	{
		QFAIL("Composition crashed.");
	}
};

/**
 * Wraps the specified node in a composition, compiles it,
 * attaches a delegate that fails the current test if the connection to the composition is lost,
 * and starts it running.
 */
VuoRunner *TestCompositionExecution::createAndStartRunnerFromNode(VuoCompilerNodeClass *nodeClass)
{
	VuoCompiler *compiler = initCompiler(QTest::currentDataTag());
	VuoCompilerIssues issues;
	string compiledCompositionPath = VuoFileUtilities::makeTmpFile(QTest::currentDataTag(), "bc");
	string linkedCompositionPath = VuoFileUtilities::makeTmpFile(QTest::currentDataTag(), "");
	compiler->compileCompositionString(TestCompositionExecution::wrapNodeInComposition(nodeClass, compiler), compiledCompositionPath, true, &issues);
	compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_ExistingModuleCaches);
	remove(compiledCompositionPath.c_str());
	delete compiler;

	VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, ".", false, true);

	if (runner)
	{
		auto delegate = new TestCompositionExecutionDelegate;
		runner->setDelegate(delegate);

		runner->setRuntimeChecking(true);
		runner->start();
	}

	return runner;
}

double TestCompositionExecution_tolerance = 0.00001;
void TestCompositionExecution::setTolerance(double tolerance)
{
	TestCompositionExecution_tolerance = tolerance;
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
	if (type == "VuoColor")
	{
		VuoColor expected = VuoColor_makeFromJson(expectedValue);
		VuoColor   actual = VuoColor_makeFromJson(actualValue);
		QVERIFY2(VuoColor_areEqualWithinTolerance(actual, expected, 0.01), failMessage.c_str());
		return;
	}
	else if (expectedType == json_type_object && actualType == json_type_object)
	{
		if (type == "VuoImage")
		{
			VuoImage expectedImage = VuoImage_makeFromJson(expectedValue);
			VuoImage   actualImage = VuoImage_makeFromJson(actualValue);
			QVERIFY2(VuoImage_areEqualWithinTolerance(actualImage, expectedImage, 1), failMessage.c_str());
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
		string elementType;
		if (VuoType::isListTypeName(type))
			elementType = VuoType::extractInnermostTypeName(type);

		int actualElementCount = json_object_array_length(actualValue);
		int expectedElementCount = json_object_array_length(expectedValue);
		QVERIFY2(actualElementCount == expectedElementCount, failMessage.c_str());

		for (int i = 0; i < expectedElementCount; ++i)
		{
			json_object *actualElement = json_object_array_get_idx(actualValue, i);
			json_object *expectedElement = json_object_array_get_idx(expectedValue, i);
			checkEqual(itemName, elementType, actualElement, expectedElement);
		}
	}
	else if ((expectedType == json_type_double && (actualType == json_type_double || actualType == json_type_int)) ||
			 (actualType == json_type_double && (expectedType == json_type_double || expectedType == json_type_int)))
	{
		double actualDouble = json_object_get_double(actualValue);
		double expectedDouble = json_object_get_double(expectedValue);
		if (isnan(actualDouble) && isnan(expectedDouble))
		{
			// OK, since NaN was both expected and actually received.
		}
		else
			QVERIFY2(fabs(actualDouble - expectedDouble) <= TestCompositionExecution_tolerance, failMessage.c_str());
	}
	else
		QVERIFY2(actualString == expectedString, failMessage.c_str());
}

/**
 * Constructs a compiler delegate that is ready to receive notifications of modules loaded/unloaded.
 */
TestCompilerDelegate::TestCompilerDelegate(void)
{
	sem = dispatch_semaphore_create(0);
	waiting = false;
	diffInfo = new VuoCompilerCompositionDiff();
}

/**
 * Destructor.
 */
TestCompilerDelegate::~TestCompilerDelegate(void)
{
	dispatch_release(sem);
	delete diffInfo;
}

/**
 * Copies the module file at @a originalPath to @a installedPath and waits for the notification that it has been installed.
 */
void TestCompilerDelegate::installModule(const string &originalPath, const string &installedPath, const string &moduleToWaitOn)
{
	waiting = true;
	moduleWaitingOn = (! moduleToWaitOn.empty() ? moduleToWaitOn : VuoCompiler::getModuleKeyForPath(installedPath));

	VuoFileUtilities::copyFile(originalPath, installedPath);

	dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
}

/**
 * Like @ref installModule, but if @a originalPath is a subcomposition source file, copies a slightly modified version
 * of it to @a installedPath.
 */
void TestCompilerDelegate::installModuleWithSuperficialChange(const string &originalPath, const string &installedPath)
{
	string dir, file, ext;
	VuoFileUtilities::splitPath(originalPath, dir, file, ext);
	if (VuoFileUtilities::isCompositionExtension(ext) || VuoFileUtilities::isCFamilySourceExtension(ext) || VuoFileUtilities::isIsfSourceExtension(ext))
	{
		string originalSource = VuoFileUtilities::readFileToString(originalPath);
		string modifiedSource = originalSource + "/* " + VuoStringUtilities::makeRandomHash(4) + " */";
		string modifiedPath = VuoFileUtilities::makeTmpFile("vuo.test.TestModuleLoading.modified", "vuo");
		VuoFileUtilities::writeStringToFile(modifiedSource, modifiedPath);

		installModule(modifiedPath, installedPath);

		VuoFileUtilities::deleteFile(modifiedPath);
	}
	else
	{
		installModule(originalPath, installedPath);
	}
}

/**
 * Overrides the module at @a installedPath with a small change to the source code and waits for the notification that
 * the module has been modified.
 */
void TestCompilerDelegate::overrideModuleWithSuperficialChange(const string &installedPath, VuoCompiler *compiler)
{
	waiting = true;
	moduleWaitingOn = VuoCompiler::getModuleKeyForPath(installedPath);

	string sourceCode = VuoFileUtilities::readFileToString(installedPath);
	sourceCode += "/* " + VuoStringUtilities::makeRandomHash(4) + " */";
	compiler->overrideInstalledNodeClass(installedPath, sourceCode);

	dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
}

/**
 * Reverts a previous override to the module's source code and waits for the notification that the module has been modified.
 */
void TestCompilerDelegate::revertOverriddenModule(const string &installedPath, VuoCompiler *compiler)
{
	waiting = true;
	moduleWaitingOn = VuoCompiler::getModuleKeyForPath(installedPath);

	compiler->revertOverriddenNodeClass(installedPath);

	dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
}

/**
 * Deletes the module file at @a installedPath and waits for the notification that it has been uninstalled.
 */
void TestCompilerDelegate::uninstallModule(const string &installedPath)
{
	waiting = true;
	moduleWaitingOn = VuoCompiler::getModuleKeyForPath(installedPath);

	VuoFileUtilities::deleteFile(installedPath);

	dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
}

/**
 * Handles a notification that modules have been installed/uninstalled.
 */
void TestCompilerDelegate::loadedModules(const map<string, VuoCompilerModule *> &modulesAdded,
										 const map<string, pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified,
										 const map<string, VuoCompilerModule *> &modulesRemoved, VuoCompilerIssues *issues)
{
	bool found = false;
	if (waiting)
	{
		for (auto m : modulesAdded)
		{
			if (m.first == moduleWaitingOn)
			{
				found = true;
				break;
			}
		}
		for (auto m : modulesModified)
		{
			if (m.first == moduleWaitingOn)
			{
				found = true;
				break;
			}
		}
		for (auto m : modulesRemoved)
		{
			if (m.first == moduleWaitingOn)
			{
				found = true;
				break;
			}
		}
	}

	for (auto m : modulesModified)
	{
		VuoCompilerNodeClass *oldNodeClass = dynamic_cast<VuoCompilerNodeClass *>(m.second.first);
		VuoCompilerNodeClass *newNodeClass = dynamic_cast<VuoCompilerNodeClass *>(m.second.second);
		if (oldNodeClass && newNodeClass)
			diffInfo->addNodeClassReplacement(oldNodeClass, newNodeClass);
		else
			diffInfo->addModuleReplacement(m.first);
	}

	if (found)
	{
		dispatch_semaphore_signal(sem);
		waiting = false;
		moduleWaitingOn = "";
	}

	if (! issues->isEmpty())
		QFAIL(issues->getLongDescription(false).c_str());

	loadedModulesCompleted();
}

VuoCompilerCompositionDiff * TestCompilerDelegate::takeDiffInfo(void)
{
	VuoCompilerCompositionDiff *ret = diffInfo;
	diffInfo = new VuoCompilerCompositionDiff();
	return ret;
}
