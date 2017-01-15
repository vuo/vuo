/**
 * @file
 * TestVuoRenderer implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererPort.hh"
#include "VuoPortClass.hh"
#include "VuoPort.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoRendererComposition.hh"
#include "VuoRendererNode.hh"
#include "VuoRendererCable.hh"
#include "VuoRendererTypecastPort.hh"
#include "VuoCompilerCable.hh"
#include "VuoRendererSignaler.hh"

/**
 * Tests for the VuoRenderer* classes.
 */
class TestVuoRenderer : public QObject
{
	Q_OBJECT

public:
	TestVuoRenderer()
	{
		compiler = NULL;
	}

private:
	VuoCompiler *compiler; ///< A compiler instance for loading node classes.

private slots:
	void initTestCase()
	{
		compiler = new VuoCompiler();

		// Make sure fonts get loaded before any tests run.
		VuoComposition *baseComposition = new VuoComposition();
		VuoRendererComposition *composition = new VuoRendererComposition(baseComposition);
		delete composition;
		delete baseComposition;
	}

	void cleanupTestCase()
	{
		delete compiler;
		compiler = NULL;
	}

	void testPortNameBoundingBox_data()
	{
		QTest::addColumn<QString>("portName");
		QTest::addColumn<qreal>("expectedWidth");

		QTest::newRow("") << "0" << 5.8;
		QTest::newRow("") << "1" << 5.8;
		QTest::newRow("") << "out0" << 21.2;
		QTest::newRow("") << "out1" << 21.2;
	}
	void testPortNameBoundingBox()
	{
		QFETCH(QString, portName);
		QFETCH(qreal, expectedWidth);

		VuoPortClass * pc = new VuoPortClass(portName.toStdString(),VuoPortClass::dataAndEventPort);
		VuoPort * p = new VuoPort(pc);
		VuoRendererPort * rp = new VuoRendererPort(p, new VuoRendererSignaler(), true, false, false);
		qreal portNameWidth = rp->getNameRect().width();
		QVERIFY2( portNameWidth >= expectedWidth,
			QString("Bounding box width %1 isn't sufficient to draw string '%2'")
			.arg(portNameWidth)
			.arg(portName)
			.toUtf8().data());
		delete rp;
		delete p;
		delete pc;
	}


	void testPortsIntersectNodeEdges_data()
	{
		QTest::addColumn<QString>("className");

		QTest::newRow("interface") << "vuo.scene.render.window";
		QTest::newRow("stateful") << "vuo.time.firePeriodically";
		QTest::newRow("stateless") << "vuo.math.add.VuoInteger";
	}
	void testPortsIntersectNodeEdges()
	{
		QFETCH(QString, className);

		VuoCompilerNodeClass * cnc = compiler->getNodeClass(className.toStdString());
		VuoNode * n = cnc->newNode();
		VuoRendererNode * rn = new VuoRendererNode(n, new VuoRendererSignaler());
		QRectF nodeRect = rn->boundingRect();
		foreach (VuoRendererPort * p, rn->getInputPorts()->childItems())
			QVERIFY2(nodeRect.intersects(p->boundingRect()), "An input port isn't visually connected to its node");
		foreach (VuoRendererPort * p, rn->getOutputPorts()->childItems())
			QVERIFY2(nodeRect.intersects(p->boundingRect()), "An output port isn't visually connected to its node");

		delete rn;
		delete n;
	}


	void testNodeWideEnoughForPort_data()
	{
		QTest::addColumn<QString>("portDisplayName");

		QTest::newRow("time") << "Time";
		QTest::newRow("long") << "Node Instance Trigger Stop Start 123";
	}
	void testNodeWideEnoughForPort()
	{
		QFETCH(QString, portDisplayName);

		vector<string> inputs, outputs;
		inputs.push_back("i");
		inputs.push_back(portDisplayName.toStdString());
		VuoNodeClass *nc = new VuoNodeClass("test", inputs, outputs);
		VuoNode *n = nc->newNode();
		VuoRendererNode *rn = new VuoRendererNode(n, new VuoRendererSignaler());
		VuoRendererPortList *rp = rn->getInputPorts();

		float nodeRight = rn->boundingRect().right();
		float portRight = rp->childItems().at(2)->boundingRect().right();
		QVERIFY(portRight < nodeRight);

		delete rn;
		delete n;
		delete nc;
	}


	void testCableCarriesData_data()
	{
		QTest::addColumn<QString>("fromNode");
		QTest::addColumn<QString>("fromPort");
		QTest::addColumn<QString>("toNode");
		QTest::addColumn<QString>("toPort");
		QTest::addColumn<bool>("expectedCarriesData");

		QTest::newRow("data to data")						<< "vuo.text.cut"			<< "partialText"	<< "vuo.console.window"		<< "writeLine"	<< true;
		QTest::newRow("data to event")						<< "vuo.text.cut"			<< "partialText"	<< "vuo.select.in.event.2"	<< "option1"		<< false;
		QTest::newRow("data to refresh")					<< "vuo.text.cut"			<< "partialText"	<< "vuo.event.fireOnStart"	<< "refresh"		<< false;
		QTest::newRow("event to data")						<< "vuo.select.in.event.2"	<< "out"			<< "vuo.console.window"		<< "writeLine"	<< false;
		QTest::newRow("event to event")						<< "vuo.select.in.event.2"	<< "out"			<< "vuo.select.in.event.2"	<< "option1"		<< false;
		QTest::newRow("event to refresh")					<< "vuo.select.in.event.2"	<< "out"			<< "vuo.event.fireOnStart"	<< "refresh"		<< false;
		QTest::newRow("trigger with data to data")			<< "vuo.console.window"		<< "typedLine"		<< "vuo.console.window"		<< "writeLine"	<< true;
		QTest::newRow("trigger with data to event")			<< "vuo.console.window"		<< "typedLine"		<< "vuo.select.in.event.2"	<< "option1"		<< false;
		QTest::newRow("trigger with data to refresh")		<< "vuo.console.window"		<< "typedLine"		<< "vuo.event.fireOnStart"	<< "refresh"		<< false;
		QTest::newRow("trigger without data to data")		<< "vuo.event.fireOnStart"	<< "started"		<< "vuo.console.window"		<< "writeLine"	<< false;
		QTest::newRow("trigger without data to event")		<< "vuo.event.fireOnStart"	<< "started"		<< "vuo.select.in.event.2"	<< "option1"		<< false;
		QTest::newRow("trigger without data to refresh")	<< "vuo.event.fireOnStart"	<< "started"		<< "vuo.event.fireOnStart"	<< "refresh"		<< false;
	}
	void testCableCarriesData()
	{
		QFETCH(QString, fromNode);
		QFETCH(QString, fromPort);
		QFETCH(QString, toNode);
		QFETCH(QString, toPort);
		QFETCH(bool, expectedCarriesData);

		VuoNode * fromN = compiler->getNodeClass(fromNode.toStdString())->newNode();
		VuoCompilerPort * fromCP = (VuoCompilerPort *) fromN->getOutputPortWithName(fromPort.toStdString())->getCompiler();
		VuoNode * toN = compiler->getNodeClass(toNode.toStdString())->newNode();
		VuoCompilerPort * toCP = (VuoCompilerPort *) toN->getInputPortWithName(toPort.toStdString())->getCompiler();

		VuoCompilerCable * cc = new VuoCompilerCable(fromN->getCompiler(), fromCP, toN->getCompiler(), toCP);
		QCOMPARE(cc->carriesData(), expectedCarriesData);
		delete cc;

		delete toN;
		delete fromN;
	}


	void testTypecastCollapsing()
	{
		VuoCompiler * compiler = new VuoCompiler();

		VuoComposition * baseComposition = new VuoComposition();
		VuoRendererComposition * composition = new VuoRendererComposition(baseComposition);

		VuoNode * nodeOutputtingData = compiler->getNodeClass("vuo.select.out.2.VuoInteger")->newNode();
		composition->addNode(nodeOutputtingData);
		VuoPort * portOutputtingData = nodeOutputtingData->getOutputPortWithName("option1");

		VuoNode * typecast = compiler->getNodeClass("vuo.type.integer.text")->newNode();
		composition->addNode(typecast);
		VuoPort * typecastInPort = typecast->getInputPortWithName("integer");
		VuoPort * typecastOutPort = typecast->getOutputPortWithName("text");

		VuoNode * write = compiler->getNodeClass("vuo.console.window")->newNode();
		composition->addNode(write);
		VuoPort * writePort = write->getInputPortWithName("writeLine");

		VuoCable * cableCarryingData = (new VuoCompilerCable(nodeOutputtingData->getCompiler(),
																(VuoCompilerPort *)portOutputtingData->getCompiler(),
																typecast->getCompiler(),
																(VuoCompilerPort *)typecastInPort->getCompiler()))->getBase();
		composition->addCable(cableCarryingData);

		VuoCable * cable = (new VuoCompilerCable(typecast->getCompiler(),
												(VuoCompilerPort *)typecastOutPort->getCompiler(),
												write->getCompiler(),
												(VuoCompilerPort *)writePort->getCompiler()))->getBase();
		composition->addCable(cable);

		int expectedNumInputPorts = 2;
		QCOMPARE(write->getInputPorts().size(), (size_t)expectedNumInputPorts);
		QCOMPARE(write->getRenderer()->getInputPorts()->childItems().size(), expectedNumInputPorts);

		composition->collapseTypecastNodes();

		QVERIFY2(typecast->getRenderer()->getProxyNode() != NULL, "The typecast node should have a rendering proxy.");

		VuoRendererTypecastPort * writePortTypecast = dynamic_cast<VuoRendererTypecastPort *>(writePort->getRenderer());
		QVERIFY2(writePortTypecast != NULL, "The target node's input port should be a typecast port.");
		QVERIFY2(writePortTypecast->getChildPort() != NULL, "The target node's typecast port should have a child port.");

		QCOMPARE(write->getInputPorts().size(), (size_t)expectedNumInputPorts);
		QCOMPARE(write->getRenderer()->getInputPorts()->childItems().size(), expectedNumInputPorts);

		delete composition;
		delete baseComposition;
		delete compiler;
	}
};

int main(int argc, char *argv[])
{
	VuoRendererComposition::createAutoreleasePool();

	// https://bugreports.qt-project.org/browse/QTBUG-29197
	qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1");

	QApplication app(argc, argv);
	TestVuoRenderer tc;
	return QTest::qExec(&tc, argc, argv);
}

#include "TestVuoRenderer.moc"
