/**
 * @file
 * TestReferenceCounting interface and implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "TestCompositionExecution.hh"


/**
 * A runner delegate that fires a series of events through a composition and checks for errors.
 */
class TestReferenceCountingRunnerDelegate : public TestRunnerDelegate
{
private:
	VuoRunner *runner;  ///< The runner created and used by this runner delegate.
	VuoRunner::Port *inEventPort;  ///< The input port through which to fire events.
	VuoRunner::Port *outEventPort;  ///< The output port through which to receive events.
	int outEventCount;  ///< The number of events received so far through outEventPort.

public:
	vector<string> errorMessages;  ///< The list of error messages received via telemetry.

	/**
	 * Creates a TestReferenceCountingRunnerDelegate, starts the composition, fires the first event,
	 * and waits until the composition stops.
	 */
	TestReferenceCountingRunnerDelegate(string compositionPath, VuoCompiler *compiler)
	{
		outEventCount = 0;

		string compositionDir, file, ext;
		VuoFileUtilities::splitPath(compositionPath, compositionDir, file, ext);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile("VuoRunnerComposition", "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile("VuoRunnerComposition-linked", "");
		compiler->compileComposition(compositionPath, compiledCompositionPath);
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_FastBuild);
		remove(compiledCompositionPath.c_str());
		runner = VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, compositionDir, false, true);

		runner->setDelegate(this);
		runner->start();

		inEventPort = runner->getPublishedInputPortWithName("inEvent");
		outEventPort = runner->getPublishedOutputPortWithName("outEvent");

		runner->firePublishedInputPortEvent(inEventPort);

		runner->waitUntilStopped();
	}

	/**
	 * If enough events have been received, stops the composition. Otherwise, fires another event.
	 */
	void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port *port, bool sentData, string dataSummary)
	{
		if (port != outEventPort)
			return;

		if (++outEventCount < 100)
		{
			runner->firePublishedInputPortEvent(inEventPort);
		}
		else
		{
			// runner->stop() has to be called asynchronously because it waits for this function to return.
			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							   runner->stop();
						   });
		}
	}

	/**
	 * Adds the error message to the list.
	 */
	void receivedTelemetryError(string message)
	{
		errorMessages.push_back(message);
	}

	~TestReferenceCountingRunnerDelegate()
	{
		delete runner;
	}
};


/**
 * Tests the reference-counting system for port and node instance data.
 */
class TestReferenceCounting : public TestCompositionExecution
{
	Q_OBJECT

private slots:

	void testTelemetryErrors_data()
	{
		QTest::addColumn< QString >("compositionFile");
		QTest::addColumn< QString >("expectedErrorSubstring");

		QTest::newRow("VuoRegister called on an already-registered pointer") << "RegisterTwice.vuo" << "VuoRegister was called more than once";
		QTest::newRow("VuoRetain called on a never-registered pointer") << "RetainWithoutRegister.vuo" << "for unregistered pointer";
		QTest::newRow("VuoRelease called on a never-registered pointer") << "ReleaseWithoutRegister.vuo" << "for unregistered pointer";
		QTest::newRow("VuoRelease called on an already-released pointer that was never retained") << "ReleaseWithoutAnyRetains.vuo" << "for unretained pointer";
		QTest::newRow("VuoRelease called on an already-released pointer that was previously retained") << "ReleaseWithoutEnoughRetains.vuo" << "for unregistered pointer";
		QTest::newRow("VuoRelease not called enough times") << "RetainWithoutRelease.vuo" << "VuoRelease was not called enough times";
		QTest::newRow("VuoRegister called without retains or releases") << "RegisterOnly.vuo" << "VuoRelease was not called enough times";
		QTest::newRow("VuoRegister called on a null pointer") << "RegisterNull.vuo" << "";
		QTest::newRow("VuoRetain called on a null pointer") << "RetainNull.vuo" << "";
		QTest::newRow("VuoRelease called on a null pointer") << "ReleaseNull.vuo" << "";
		QTest::newRow("VuoString stored only in a constant input port") << "CountCharacters.vuo" << "";
		QTest::newRow("VuoString stored only in an unpublished output port") << "OutputStringAndEvent.vuo" << "";
		QTest::newRow("VuoString stored only in a published output port") << "ConvertIntegerToText.vuo" << "";
		QTest::newRow("VuoString stored only in simple node instance data") << "StoreString.vuo" << "";
		QTest::newRow("VuoString stored only in compound node instance data") << "StoreStructOfStrings.vuo" << "";
		QTest::newRow("VuoString carried through a cable on every event") << "TextThroughCable.vuo" << "";
		QTest::newRow("VuoString carried through a cable on some events") << "TextThroughCableAlternating.vuo" << "";
		QTest::newRow("VuoString carried through a scatter of cables") << "ScatterText.vuo" << "";
		QTest::newRow("VuoString carried through a scatter and gather of cables") << "ScatterGatherText.vuo" << "";
		QTest::newRow("VuoString carried through multiple scatters and gathers of cables") << "MultipleScatterGatherText.vuo" << "";
		QTest::newRow("VuoString stored in the first/last node of a feedback loop") << "TextFeedback.vuo" << "";
		QTest::newRow("Empty VuoList of VuoStrings stored only in a constant input port") << "InputList.vuo" << "";
		QTest::newRow("Non-empty VuoList of VuoStrings stored only in an unpublished output port") << "OutputList.vuo" << "";
		QTest::newRow("VuoList_VuoSceneObject stored only a published output port") << "MakeListOfSceneObjects.vuo" << "";
	}
	void testTelemetryErrors()
	{
		QFETCH(QString, compositionFile);
		QFETCH(QString, expectedErrorSubstring);
		printf("	%s\n", compositionFile.toUtf8().data()); fflush(stdout);

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		VuoCompiler *compiler = initCompiler();
		TestReferenceCountingRunnerDelegate delegate(compositionPath, compiler);

		if (! delegate.errorMessages.empty())
		{
			for (vector<string>::iterator i = delegate.errorMessages.begin(); i != delegate.errorMessages.end(); ++i)
			{
				QString actualError = QString((*i).c_str());
				if (expectedErrorSubstring.isEmpty() || ! actualError.contains(expectedErrorSubstring))
				{
					QString expectedResult = (expectedErrorSubstring.isEmpty() ?
												  QString("no errors") :
												  QString("error '%1'").arg(expectedErrorSubstring));
					QFAIL(qPrintable(QString("Expected %1, but got unexpected error '%2'.").arg(expectedResult).arg(actualError)));
				}
			}
		}
		else
		{
			if (! expectedErrorSubstring.isEmpty())
				QFAIL(qPrintable(QString("Expected error '%1', but got no errors.").arg(expectedErrorSubstring)));
		}

		delete compiler;
	}
};

QTEST_APPLESS_MAIN(TestReferenceCounting)
#include "TestReferenceCounting.moc"
