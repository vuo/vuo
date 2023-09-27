/**
 * @file
 * TestSceneObjectGenerators implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "TestCompositionExecution.hh"
#include <Vuo/Vuo.h>

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoCompilerNodeClass *);
Q_DECLARE_METATYPE(string);

/**
 * Tests each scene object generator node for common mistakes.
 */
class TestSceneObjectGenerators : public TestCompositionExecution
{
	Q_OBJECT

private:
	int argc;
	char **argv;

public:

	TestSceneObjectGenerators(int argc, char **argv)
		: argc(argc),
		  argv(argv)
	{
	}

private slots:

	/**
	 * Ensures each scene object generator doesn't crash and outputs a valid object
	 * when there are 0 rows/columns/slices/subdivisions.
	 *
	 * https://b33p.net/kosada/vuo/vuo/-/issues/19601
	 */
	void testZeroSubdivisions_data()
	{
		QTest::addColumn<VuoCompilerNodeClass *>("nodeClass");
		QTest::addColumn<string>("outputPort");

		VuoCompiler *compiler = initCompiler("TestSceneObjectGenerators/testZeroSubdivisions");
		VuoDefer(^{ delete compiler; });

		if (argc > 1)
		{
			// If a single test is specified on the command line,
			// improve performance by loading only data for that test.
			QString nodeClassName(QString::fromUtf8(argv[1]).section(':', 1));
			if (!nodeClassName.isEmpty())
			{
				VuoCompilerNodeClass *nc = compiler->getNodeClass(nodeClassName.toStdString());
				VuoPortClass *soOutput = getFirstPortOfType(nc->getBase()->getOutputPortClasses(), "VuoSceneObject");
				QTest::newRow(nodeClassName.toUtf8().constData()) << nc << soOutput->getName();
				return;
			}
		}

		for (auto &nc : compiler->getNodeClasses())
		{
			if (!VuoStringUtilities::beginsWith(nc.first, "vuo."))
				continue;

			VuoPortClass *soInput  = getFirstPortOfType(nc.second->getBase()->getInputPortClasses(), "VuoSceneObject");
			if (soInput)
				// If it has a scene object input port, it's probably a group or scene object filter; no need to run this test on those nodes.
				continue;

			VuoPortClass *intInput = getFirstPortOfType(nc.second->getBase()->getInputPortClasses(), "VuoInteger");
			if (!intInput)
				// Only test nodes that have an integer input (probably rows/columns/slices/subdivisions/etc).
				continue;

			VuoPortClass *soOutput = getFirstPortOfType(nc.second->getBase()->getOutputPortClasses(), "VuoSceneObject");
			if (!soOutput)
				// Only test nodes that output a scene object.
				continue;

			QTest::newRow(nc.first.c_str()) << nc.second << soOutput->getName();
		}
	}
	void testZeroSubdivisions()
	{
		QFETCH(VuoCompilerNodeClass *, nodeClass);
		QFETCH(string, outputPort);

		VuoRunner *runner = createAndStartRunnerFromNode(nodeClass);
		QVERIFY(runner);

		// Set all integer input ports to 0.
		map<VuoRunner::Port *, json_object *> portsToSet;
		VuoRunner::Port *somePort = nullptr;
		for (auto port : runner->getPublishedInputPorts())
			if (port->getType() == "VuoInteger")
			{
				portsToSet[port] = json_object_new_int(0);
				somePort = port;
			}
		runner->setPublishedInputPortValues(portsToSet);

		// Execute the node.
		runner->firePublishedInputPortEvent(somePort);
		runner->waitForFiredPublishedInputPortEvent();

		// Ensure that the composition doesn't crash, and that it outputs something (rather than a null or empty object).
		VuoRunner::Port *p = runner->getPublishedOutputPortWithName(outputPort);
		json_object *out = runner->getPublishedOutputPortValue(p);

		QCOMPARE(json_object_get_type(out), json_type_object);

		json_object *o;
		QVERIFY(json_object_object_get_ex(out, "type", &o));
		QVERIFY(QString(json_object_get_string(o)) != "empty");

		runner->stop();
		delete runner;
	}
};

int main(int argc, char *argv[])
{
	TestSceneObjectGenerators tc(argc, argv);
	return QTest::qExec(&tc, argc, argv);
}

#include "TestSceneObjectGenerators.moc"
