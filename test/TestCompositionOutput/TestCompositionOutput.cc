/**
 * @file
 * TestCompositionOutput interface and implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
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
class TestCompositionOutputRunnerDelegate : public TestRunnerDelegate
{
private:
	VuoRunner *runner;  ///< The runner created and used by this TestCompositionOutputRunnerDelegate.
	list<PortConfiguration *> portConfigurations;  ///< The sequence of PortConfiguration objects to apply to the running composition.

	/**
	 * Applies each PortConfiguration in the list until it has applied one with a
	 * non-empty set of expected output port values.
	 */
	void setInputValuesAndFireEventsUntilCheckIsNeeded(void)
	{
		while (true)
		{
			PortConfiguration *currentPortConfiguration = portConfigurations.front();
			currentPortConfiguration->setInputValuesAndFireEvent(runner);
			if (! currentPortConfiguration->isDoneChecking())
				break;
			portConfigurations.pop_front();

			QVERIFY2(! portConfigurations.empty(), "The last PortConfiguration should have expected output port values.");
		}
	}

public:

	/**
	 * Creates a TestCompositionOutputRunnerDelegate, starts the composition, fires the first event,
	 * and waits until the composition stops.
	 */
	TestCompositionOutputRunnerDelegate(string composition, list<PortConfiguration *> portConfigurations, VuoCompiler *compiler)
	{
		runner = NULL;
		this->portConfigurations = portConfigurations;

		string compiledCompositionPath = VuoFileUtilities::makeTmpFile("VuoRunnerComposition", "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile("VuoRunnerComposition-linked", "");
		compiler->compileCompositionString(composition, compiledCompositionPath);
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath);
		remove(compiledCompositionPath.c_str());
		runner = VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, true);

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
		if (! sentData)
			return;

		QVERIFY(! portConfigurations.empty());
		PortConfiguration *currentPortConfiguration = portConfigurations.front();
		currentPortConfiguration->checkOutputValue(port, dataSummary);

		if (currentPortConfiguration->isDoneChecking())
		{
			portConfigurations.pop_front();

			if (portConfigurations.empty())
			{
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
	}

	~TestCompositionOutputRunnerDelegate()
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
class TestCompositionOutput : public TestCompositionExecution
{
	Q_OBJECT

private:

	VuoCompiler *compiler;

	/**
	 * Returns a string containing a composition that consists of an instance of a node class
	 * with all its input and output ports published.
	 */
	string wrapNodeInComposition(string nodeClassName)
	{
		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName);
		if (! nodeClass)
			return "";

		VuoNode *node = nodeClass->newNode();

		VuoComposition baseComposition;
		VuoCompilerComposition composition(&baseComposition, NULL);
		baseComposition.addNode(node);

		vector<VuoPort *> inputPorts = node->getInputPorts();
		vector<VuoPort *> outputPorts = node->getOutputPorts();
		for (int i = 0; i < inputPorts.size(); ++i)
		{
			VuoPort *port = inputPorts.at(i);
			set<VuoPort *> connectedPorts;
			connectedPorts.insert(port);
			VuoPublishedPort *publishedPort = new VuoPublishedPort(port->getClass()->getName(), NULL, false, connectedPorts);
			baseComposition.addPublishedInputPort(publishedPort, i);
		}
		for (int i = 0; i < outputPorts.size(); ++i)
		{
			VuoPort *port = outputPorts.at(i);
			set<VuoPort *> connectedPorts;
			connectedPorts.insert(port);
			VuoPublishedPort *publishedPort = new VuoPublishedPort(port->getClass()->getName(), NULL, true, connectedPorts);
			baseComposition.addPublishedOutputPort(publishedPort, i);
		}

		return composition.getGraphvizDeclaration();
	}

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

	void testBuildCompositionFromNode_data()
	{
		QTest::addColumn< QString >("nodeClassName");

		QList<QString> nodeClassNames;
		nodeClassNames.append("vuo.console.window");
		nodeClassNames.append("vuo.image.filter.ripple");
		nodeClassNames.append("vuo.image.filter.twirl");
		nodeClassNames.append("vuo.image.get");
		nodeClassNames.append("vuo.image.render.window");
		nodeClassNames.append("vuo.noise.gradient.1d");
		nodeClassNames.append("vuo.noise.gradient.2d");
		nodeClassNames.append("vuo.noise.gradient.3d");
		nodeClassNames.append("vuo.noise.gradient.4d");
		nodeClassNames.append("vuo.midi.listDevices");
		nodeClassNames.append("vuo.midi.receive");
		nodeClassNames.append("vuo.midi.send");
		nodeClassNames.append("vuo.mouse");
		nodeClassNames.append("vuo.scene.get");
		nodeClassNames.append("vuo.scene.make");
		nodeClassNames.append("vuo.scene.make.cube");
		nodeClassNames.append("vuo.scene.make.image");
		nodeClassNames.append("vuo.scene.render.image");
		nodeClassNames.append("vuo.scene.render.window");
		nodeClassNames.append("vuo.shader.make.color");
		nodeClassNames.append("vuo.shader.make.image");
		nodeClassNames.append("vuo.shader.make.normal");
		nodeClassNames.append("vuo.text.append");
		nodeClassNames.append("vuo.vertices.make.parametric");
		nodeClassNames.append("vuo.vertices.make.sphere");
		nodeClassNames.append("vuo.vertices.make.square");
		nodeClassNames.append("vuo.vertices.make.triangle");

		foreach (QString n, nodeClassNames)
			QTest::newRow(qPrintable(n)) << n;
	}
	void testBuildCompositionFromNode()
	{
		QFETCH(QString, nodeClassName);

		string composition = wrapNodeInComposition(nodeClassName.toStdString());
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile("VuoRunnerComposition", "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile("VuoRunnerComposition-linked", "");

		remove(compiledCompositionPath.c_str());
		remove(linkedCompositionPath.c_str());

		compiler->compileCompositionString(composition, compiledCompositionPath);
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath);

		ifstream file(linkedCompositionPath.c_str());
		QVERIFY(file);
		file.close();

		remove(compiledCompositionPath.c_str());
		remove(linkedCompositionPath.c_str());
	}

	void testPublishedOutputsForPublishedInputs_data()
	{
		QTest::addColumn< string >("sourcePath");
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

			string sourcePath;
			string composition;
			if (QFile(compositionPath.c_str()).exists())
			{
				sourcePath = compositionPath;
				ifstream fin(compositionPath.c_str());
				composition.append( (istreambuf_iterator<char>(fin)), (istreambuf_iterator<char>()) );
				fin.close();
			}
			else
			{
				sourcePath = name;
				composition = wrapNodeInComposition(name);
			}

			QTest::newRow(name.c_str()) << sourcePath << portConfigurationPath << composition;
		}
	}
	void testPublishedOutputsForPublishedInputs()
	{
		QFETCH(string, sourcePath);
		QFETCH(string, portConfigurationPath);
		QFETCH(string, composition);

		list<PortConfiguration *> portConfigurations;
		PortConfiguration::readListFromJSONFile(portConfigurationPath, portConfigurations);

		fprintf(stderr, "	%s\n", sourcePath.c_str());
		QVERIFY2(! composition.empty(), sourcePath.c_str());
		TestCompositionOutputRunnerDelegate delegate(composition, portConfigurations, compiler);
	}

};

QTEST_APPLESS_MAIN(TestCompositionOutput)
#include "TestCompositionOutput.moc"
