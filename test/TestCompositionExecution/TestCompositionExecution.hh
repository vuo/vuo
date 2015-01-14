/**
 * @file
 * TestCompositionExecution interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef TESTCOMPOSITIONEXECUTION_H
#define TESTCOMPOSITIONEXECUTION_H

#include <fstream>
#include <QtTest>
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

#endif
