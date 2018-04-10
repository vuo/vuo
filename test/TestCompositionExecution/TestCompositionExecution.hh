/**
 * @file
 * TestCompositionExecution interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
	static void printMemoryUsage(string label);

protected:
	static VuoCompiler * initCompiler(void);

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
