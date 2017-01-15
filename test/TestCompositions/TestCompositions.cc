/**
 * @file
 * TestCompositions interface and implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>
#include <Vuo/Vuo.h>

#include "TestCompositionExecution.hh"
#include "PortConfiguration.hh"

/**
 * A runner delegate that executes a composition with a sequence of PortConfiguration objects.
 */
class TestCompositionsRunnerDelegate : public TestRunnerDelegate
{
private:
	VuoRunner *runner;  ///< The runner created and used by this TestCompositionsRunnerDelegate.
	list<PortConfiguration *> portConfigurations;  ///< The sequence of PortConfiguration objects to apply to the running composition.
	dispatch_semaphore_t portConfigurationsSemaphore;
	bool isStopping;

	/**
	 * Applies each PortConfiguration in the list until it has applied one with a
	 * non-empty set of expected output port values.
	 */
	void setInputValuesAndFireEventsUntilCheckIsNeeded(void)
	{
		dispatch_semaphore_wait(portConfigurationsSemaphore, DISPATCH_TIME_FOREVER);
		while (true)
		{
			PortConfiguration *currentPortConfiguration = portConfigurations.front();
			currentPortConfiguration->setInputValuesAndFireEvent(runner);
			if (! currentPortConfiguration->isDoneChecking())
				break;
			portConfigurations.pop_front();

			QVERIFY2(! portConfigurations.empty(), "The last PortConfiguration should have expected output port values.");
		}
		dispatch_semaphore_signal(portConfigurationsSemaphore);
	}

public:

	/**
	 * Creates a TestCompositionsRunnerDelegate, starts the composition, fires the first event,
	 * and waits until the composition stops.
	 */
	TestCompositionsRunnerDelegate(string composition, string compositionDir, list<PortConfiguration *> portConfigurations, VuoCompiler *compiler)
	{
		runner = NULL;
		this->portConfigurations = portConfigurations;
		portConfigurationsSemaphore = dispatch_semaphore_create(1);
		isStopping = false;

		string compiledCompositionPath = VuoFileUtilities::makeTmpFile("VuoRunnerComposition", "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile("VuoRunnerComposition-linked", "");
		compiler->compileCompositionString(composition, compiledCompositionPath);
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_FastBuild);
		remove(compiledCompositionPath.c_str());
		runner = VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, compositionDir, false, true);

		runner->setDelegate(this);

		runner->start();

		QVERIFY(! portConfigurations.empty());
		setInputValuesAndFireEventsUntilCheckIsNeeded();

		runner->waitUntilStopped();
	}

	/**
	 * Checks the published output port value against the current PortConfiguration. If this is the last published
	 * output port to be checked for this event, either fires another event (if more remain) or stops the composition.
	 */
	void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port *port, bool sentData, string dataSummary)
	{
		if (! sentData || isStopping)
			return;

		dispatch_semaphore_wait(portConfigurationsSemaphore, DISPATCH_TIME_FOREVER);
		QVERIFY(! portConfigurations.empty());
		PortConfiguration *currentPortConfiguration = portConfigurations.front();
		currentPortConfiguration->checkOutputValue(runner, port);

		if (currentPortConfiguration->isDoneChecking())
		{
			portConfigurations.pop_front();

			bool empty = portConfigurations.empty();
			dispatch_semaphore_signal(portConfigurationsSemaphore);
			if (empty)
			{
				isStopping = true;

				// runner->stop() has to be called asynchronously because it waits for this function to return.
				dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
								   runner->stop();
							   });
			}
			else
			{
				setInputValuesAndFireEventsUntilCheckIsNeeded();
			}
		}
		else
		{
			dispatch_semaphore_signal(portConfigurationsSemaphore);
		}
	}

	~TestCompositionsRunnerDelegate()
	{
		delete runner;
	}
};


// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(string);
Q_DECLARE_METATYPE(vector<string>);


/**
 * Tests that compositions give correct output when executed.
 */
class TestCompositions : public TestCompositionExecution
{
	Q_OBJECT

private:

	VuoCompiler *compiler;

private slots:

	void initTestCase()
	{
		compiler = initCompiler();
	}

	void cleanupTestCase()
	{
		delete compiler;
	}

	void testReadPortConfigurationsFromJSONFile()
	{
		string portConfigurationPath = getCompositionPath("ConvertFahrenheitToCelsius.json");
		list<PortConfiguration *> portConfigurations;
		PortConfiguration::readListFromJSONFile(portConfigurationPath, portConfigurations);

		QCOMPARE(portConfigurations.size(), (size_t) 2);

		{
			string firingPortName = "degreesFahrenheit";
			map<string, string> valueForInputPortName;
			valueForInputPortName["degreesFahrenheit"] = "32";
			map<string, string> valueForOutputPortName;
			valueForOutputPortName["degreesCelsius"] = "0";
			PortConfiguration expected(firingPortName, valueForInputPortName, valueForOutputPortName);
			portConfigurations.front()->checkEqual(expected);
		}

		{
			string firingPortName = "degreesFahrenheit";
			map<string, string> valueForInputPortName;
			valueForInputPortName["degreesFahrenheit"] = "212";
			map<string, string> valueForOutputPortName;
			valueForOutputPortName["degreesCelsius"] = "100";
			PortConfiguration expected(firingPortName, valueForInputPortName, valueForOutputPortName);
			portConfigurations.back()->checkEqual(expected);
		}
	}

	void testRunningCompositions_data()
	{
		QTest::addColumn< string >("portConfigurationPath");
		QTest::addColumn< string >("composition");

		QDir compositionDir = getCompositionDir();
		QStringList filter("*.json");
		QStringList portConfigurationFileNames = compositionDir.entryList(filter);
		foreach (QString portConfigurationFileName, portConfigurationFileNames)
		{
			string portConfigurationPath = getCompositionPath(portConfigurationFileName.toStdString());

			string dir, name, extension;
			VuoFileUtilities::splitPath(portConfigurationPath, dir, name, extension);
			string compositionPath = getCompositionPath(name + ".vuo");

			string composition;
			if (QFile(compositionPath.c_str()).exists())
			{
				ifstream fin(compositionPath.c_str());
				composition.append( (istreambuf_iterator<char>(fin)), (istreambuf_iterator<char>()) );
				fin.close();
			}
			else
			{
				VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(name);
				QVERIFY(nodeClass != NULL);
				composition = wrapNodeInComposition(nodeClass, compiler);
			}

			QTest::newRow(name.c_str()) << portConfigurationPath << composition;
		}
	}
	void testRunningCompositions()
	{
		QFETCH(string, portConfigurationPath);
		QFETCH(string, composition);
		printf("	%s\n", QTest::currentDataTag()); fflush(stdout);

		list<PortConfiguration *> portConfigurations;
		PortConfiguration::readListFromJSONFile(portConfigurationPath, portConfigurations);

		QVERIFY(! composition.empty());
		string compositionDir = getCompositionDir().path().toStdString();
		TestCompositionsRunnerDelegate delegate(composition, compositionDir, portConfigurations, compiler);
	}

};

QTEST_APPLESS_MAIN(TestCompositions)
#include "TestCompositions.moc"
