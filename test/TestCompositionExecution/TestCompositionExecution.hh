/**
 * @file
 * TestCompositionExecution interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <fstream>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wunreachable-code"
#pragma clang diagnostic ignored "-Winvalid-constexpr"
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
	static void installSubcomposition(string compositionPath, string nodeClassName);
	static void uninstallSubcomposition(string nodeClassName);
	static string wrapNodeInComposition(VuoCompilerNodeClass *nodeClass, VuoCompiler *compiler);
	static void printMemoryUsage(string label);

	static void setTolerance(double tolerance);
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

/**
 * Compiler delegate that waits for modules to be loaded/unloaded.
 */
class TestCompilerDelegate : public VuoCompilerDelegate
{
protected:
	dispatch_semaphore_t sem;
	bool waiting;
	string moduleWaitingOn;
	VuoCompilerCompositionDiff *diffInfo;

	virtual void loadedModules(const map<string, VuoCompilerModule *> &modulesAdded,
							   const map<string, pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified,
							   const map<string, VuoCompilerModule *> &modulesRemoved, VuoCompilerIssues *issues);

public:
	TestCompilerDelegate(void);
	virtual ~TestCompilerDelegate(void);
	virtual void installModule(const string &originalPath, const string &installedPath, const string &moduleToWaitOn="");
	virtual void installModuleWithSuperficialChange(const string &originalPath, const string &installedPath);
	virtual void uninstallModule(const string &installedPath);
	VuoCompilerCompositionDiff * takeDiffInfo(void);
};
