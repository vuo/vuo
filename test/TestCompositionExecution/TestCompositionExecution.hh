/**
 * @file
 * TestCompositionExecution interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include <fstream>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#include <QtTest/QtTest>
#pragma clang diagnostic pop

#include <Vuo/Vuo.h>

/**
 * This base class provides common methods for testing that compositions run correctly.
 */
class TestCompositionExecution : public QObject
{
	Q_OBJECT

public:
	static QDir getCompositionDir(void);
	static string getCompositionPath(string compositionFileName);
	static void installSubcomposition(string compositionPath, string nodeClassName, VuoCompiler *compiler);
	static void uninstallSubcomposition(string nodeClassName, VuoCompiler *compiler);
	static string wrapNodeInComposition(VuoCompilerNodeClass *nodeClass, VuoCompiler *compiler);
	static void waitForImageTextCacheCleanup(void);
	static void printMemoryUsage(string label);
	static void checkEqual(string itemName, string type, json_object *actualValue, json_object *expectedValue);

protected:
	static VuoCompiler * initCompiler(void);
	static const char *getJsonTypeDescription(enum json_type);
};

/**
 * Runner delegate that fails if the composition or runner reports a problem.
 */
class TestRunnerDelegate : public VuoRunnerDelegateAdapter
{
	virtual void receivedTelemetryError(string message)
	{
		QFAIL(("receivedTelemetryError: " + message).c_str());
	}

	virtual void lostContactWithComposition(void)
	{
		QFAIL("lostContactWithComposition");
	}
};
