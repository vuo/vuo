/**
 * @file
 * TestImageFilters implementation.
 *
 * @copyright Copyright © 2012–2026 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "TestCompositionExecution.hh"
#include <Vuo/Vuo.h>

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoCompilerNodeClass *);
Q_DECLARE_METATYPE(string);

/**
 * Tests each image filter node for common mistakes.
 */
class TestImageFilters : public TestCompositionExecution
{
	Q_OBJECT

private:
	int argc;
	char **argv;

	json_object *getMenuItems(VuoPortClass *portClass)
	{
		VuoCompilerPortClass *cpc = dynamic_cast<VuoCompilerPortClass *>(portClass->getCompiler());
		VuoType *dataType = cpc->getDataVuoType();
		if (!dataType)
			return nullptr;

		// Currently the only VuoCompiler method that knows about enums
		// (both hard-enums like VuoDispersion and easy-enums like vuo.image.color.cga's `palette` port)
		// is on VuoCompilerPublishedPort, so let's fabricate one of those
		// (including a connected cable, which is required for that method to introspect easy-enums).
		auto publishedPort = VuoCompilerPublishedPort::newPort("", dataType);
		auto internalPort = cpc->newPort();
		auto cable = new VuoCable(nullptr, publishedPort->getBase(), nullptr, internalPort->getBase());
		publishedPort->getBase()->addConnectedCable(cable);

		auto details = publishedPort->getDetails(true);
		json_object *menuItems = nullptr;
		json_object_object_get_ex(details, "menuItems", &menuItems);

		delete internalPort;
		delete publishedPort;
		delete cable;

		return menuItems;
	}

public:

	TestImageFilters(int argc, char **argv)
		: argc(argc),
		  argv(argv)
	{
	}

private slots:

	/**
	 * Ensures each image filter outputs an image when it gets an image,
	 * and outputs NULL when it gets NULL after that.
	 *
	 * https://b33p.net/kosada/node/12598
	 */
	void testImageFollowedByNull_data()
	{
		QTest::addColumn<VuoCompilerNodeClass *>("nodeClass");
		QTest::addColumn<string>("inputPort");
		QTest::addColumn<string>("outputPort");

		VuoCompiler *compiler = initCompiler("/TestImageFilters/testImageFollowedByNull");
		VuoDefer(^{ delete compiler; });

		if (argc > 1)
		{
			// If a single test is specified on the command line,
			// improve performance by loading only data for that test.
			QString nodeClassName(QString::fromUtf8(argv[1]).section(':', 1));
			if (!nodeClassName.isEmpty())
			{
				VuoCompilerNodeClass *nc = compiler->getNodeClass(nodeClassName.toStdString());
				VuoPortClass *imageInput  = getFirstPortOfType(nc->getBase()->getInputPortClasses(), "VuoImage");
				VuoPortClass *imageOutput = getFirstPortOfType(nc->getBase()->getOutputPortClasses(), "VuoImage");
				QTest::newRow(nodeClassName.toUtf8().constData()) << nc << imageInput->getName() << imageOutput->getName();
				return;
			}
		}

		for (auto &nc : compiler->getNodeClasses())
		{
			if (!VuoStringUtilities::beginsWith(nc.first, "vuo."))
				continue;

			VuoPortClass *imageInput  = getFirstPortOfType(nc.second->getBase()->getInputPortClasses(), "VuoImage");
			VuoPortClass *imageOutput = getFirstPortOfType(nc.second->getBase()->getOutputPortClasses(), "VuoImage");
			if (!imageInput || !imageOutput)
				continue;

			QTest::newRow(nc.first.c_str()) << nc.second << imageInput->getName() << imageOutput->getName();
		}
	}
	void testImageFollowedByNull()
	{
		QFETCH(VuoCompilerNodeClass *, nodeClass);
		QFETCH(string, inputPort);
		QFETCH(string, outputPort);

//		printf("%s\n", QTest::currentDataTag()); fflush(stdout);

		VuoRunner *runner = createAndStartRunnerFromNode(nodeClass);
		QVERIFY(runner);

		VuoRunner::Port *inputImagePort = runner->getPublishedInputPortWithName(inputPort);
		QVERIFY(inputImagePort);
		VuoRunner::Port *outputImagePort = runner->getPublishedOutputPortWithName(outputPort);
		QVERIFY(outputImagePort);

		{
			// Set a non-null image.
			VuoImage greenImage = VuoImage_makeColorImage(VuoColor_makeWithRGBA(0,1,0,1), 640, 480);
			QVERIFY(greenImage);
			VuoLocal(greenImage);

			greenImage->scaleFactor = 2;

			map<VuoRunner::Port *, json_object *> m;
			m[inputImagePort] = VuoImage_getInterprocessJson(greenImage);
			runner->setPublishedInputPortValues(m);

			// Execute the image filter.
			runner->firePublishedInputPortEvent(inputImagePort);
			runner->waitForFiredPublishedInputPortEvent();

			// Ensure the output image is non-null.
			json_object *out = runner->getPublishedOutputPortValue(outputImagePort);
			QEXPECT_FAIL("vuo.image.combine.stereo", "Combining stereo images requires both images, but we only set one of them.", Continue);
			QEXPECT_FAIL("vuo.image.project.dome", "Requires a mesh.", Continue);
			QVERIFY(out);
			if (out)
			{
				VuoImage outputImage = VuoImage_makeFromJson(out);
				QVERIFY(outputImage);
				VuoLocal(outputImage);

				// Ensure the output image has the same scale factor as the input image.
				QEXPECT_FAIL("vuo.image.feedback", "Feedback always has scaleFactor 1.", Continue);
				QCOMPARE(outputImage->scaleFactor, greenImage->scaleFactor);
			}
		}

		{
			// Set a null image.
			map<VuoRunner::Port *, json_object *> m;
			m[inputImagePort] = NULL;
			runner->setPublishedInputPortValues(m);

			// Execute the image filter.
			runner->firePublishedInputPortEvent(inputImagePort);
			runner->waitForFiredPublishedInputPortEvent();

			// Ensure the output image is null.
			json_object *out = runner->getPublishedOutputPortValue(outputImagePort);
			QEXPECT_FAIL("vuo.image.feedback", "It's OK that feedback continues to output an image when fed NULL (allows the feedback trails to continue to swoosh around).", Continue);
			QEXPECT_FAIL("vuo.image.make.shadertoy",  "It's OK that Shadertoy outputs an image when fed NULL, since the custom shader doesn't necessarily require the image.", Continue);
			QEXPECT_FAIL("vuo.image.make.shadertoy2", "It's OK that Shadertoy outputs an image when fed NULL, since the custom shader doesn't necessarily require the image.", Continue);
			QVERIFY(out == NULL);
		}

		runner->stop();
		delete runner;
	}

	/**
	 * For each image filter node, ensures that the node outputs an image for each enum value.
	 *
	 * https://b33p.net/kosada/vuo/vuo/-/merge_requests/451#note_2228041
	 */
	void testEnumValues_data()
	{
		QTest::addColumn<VuoCompilerNodeClass *>("nodeClass");
		QTest::addColumn<QStringList>("inputImagePortNames");
		QTest::addColumn<string>("inputEnumPortName");
		QTest::addColumn<string>("outputImagePortName");

		VuoCompiler *compiler = initCompiler("/TestImageFilters/testEnumValues");
		VuoDefer(^{ delete compiler; });

		auto getImagePortNames = [=] (VuoCompilerNodeClass *nc, QStringList &imageInputNames, string &imageOutputName)
		{
			vector<VuoPortClass *> imageInputs = getPortsOfType(nc->getBase()->getInputPortClasses(), "VuoImage");
			if (imageInputs.empty())
				return false;
			for (VuoPortClass *imageInput : imageInputs)
				imageInputNames.append(QString::fromStdString(imageInput->getName()));

			VuoPortClass *imageOutput = getFirstPortOfType(nc->getBase()->getOutputPortClasses(), "VuoImage");
			if (! imageOutput)
				return false;
			imageOutputName = imageOutput->getName();

			return true;
		};

		if (argc > 1)
		{
			// If a single test is specified on the command line,
			// improve performance by loading only data for that test.
			QStringList nodeClassNameAndEnumPortName(QString::fromUtf8(argv[1]).section(':', 1).split(' '));
			if (nodeClassNameAndEnumPortName.length() == 2)
			{
				QString nodeClassName(nodeClassNameAndEnumPortName[0]);
				QString enumPortName(nodeClassNameAndEnumPortName[1]);
				VuoCompilerNodeClass *nc = compiler->getNodeClass(nodeClassName.toStdString());
				QStringList imageInputNames;
				string imageOutputName;
				if (getImagePortNames(nc, imageInputNames, imageOutputName))
					QTest::newRow((nodeClassName + " " + enumPortName).toUtf8().constData()) << nc << imageInputNames << enumPortName.toStdString() << imageOutputName;
				return;
			}
		}

		for (auto &nc : compiler->getNodeClasses())
		{
			if (!VuoStringUtilities::beginsWith(nc.first, "vuo."))
				continue;

			QStringList imageInputNames;
			string imageOutputName;
			if (! getImagePortNames(nc.second, imageInputNames, imageOutputName))
				continue;

			// Add a row for each enum port on each node.
			for (auto port : nc.second->getBase()->getInputPortClasses())
			{
				auto menuItems = getMenuItems(port);
				if (menuItems)
					QTest::newRow((nc.first + " " + port->getName()).c_str()) << nc.second << imageInputNames << port->getName() << imageOutputName;
			}
		}
	}
	void testEnumValues()
	{
		QFETCH(VuoCompilerNodeClass *, nodeClass);
		QFETCH(QStringList, inputImagePortNames);
		QFETCH(string, inputEnumPortName);
		QFETCH(string, outputImagePortName);

		VuoRunner *runner = createAndStartRunnerFromNode(nodeClass);
		QVERIFY(runner);

		vector<VuoRunner::Port *> inputImagePorts;
		for (QString inputImagePortName : inputImagePortNames)
		{
			VuoRunner::Port *inputImagePort = runner->getPublishedInputPortWithName(inputImagePortName.toStdString());
			QVERIFY(inputImagePort);
			inputImagePorts.push_back(inputImagePort);
		}
		VuoRunner::Port *inputEnumPort = runner->getPublishedInputPortWithName(inputEnumPortName);
		QVERIFY(inputEnumPort);
		VuoRunner::Port *outputImagePort = runner->getPublishedOutputPortWithName(outputImagePortName);
		QVERIFY(outputImagePort);

		VuoImage greenImage = VuoImage_makeColorImage(VuoColor_makeWithRGBA(0,1,0,1), 640, 480);
		QVERIFY(greenImage);
		VuoLocal(greenImage);
		VuoImage blueImage = VuoImage_makeColorImage(VuoColor_makeWithRGBA(0,0,1,1), 640, 480);
		QVERIFY(blueImage);
		VuoLocal(blueImage);

		json_object *menuItems = nullptr;
		json_object_object_get_ex(inputEnumPort->getDetails(), "menuItems", &menuItems);
		QVERIFY(menuItems);
		int itemCount = json_object_array_length(menuItems);
		for (int i = 0; i < itemCount; ++i)
		{
			json_object *menuItem = json_object_array_get_idx(menuItems, i);
			if (json_object_get_type(menuItem) != json_type_object)
				// Skip section headers and separators.
				continue;

			json_object *value = nullptr;
			if (!json_object_object_get_ex(menuItem, "value", &value))
				QFAIL(("an entry in the " + inputEnumPortName + " port's menuItems is missing the 'value' key").c_str());

			json_object *name = nullptr;
			if (!json_object_object_get_ex(menuItem, "name", &name))
				QFAIL(("an entry in the " + inputEnumPortName + " port's menuItems is missing the 'name' key").c_str());

			map<VuoRunner::Port *, json_object *> ports;
			for (size_t i = 0; i < inputImagePorts.size(); ++i)
				ports[inputImagePorts[i]] = VuoImage_getInterprocessJson(i%2 ? blueImage : greenImage);

			ports[inputEnumPort] = value;
			runner->setPublishedInputPortValues(ports);
			runner->firePublishedInputPortEvent(inputEnumPort);
			runner->waitForFiredPublishedInputPortEvent();

			json_object *out = runner->getPublishedOutputPortValue(outputImagePort);
			QVERIFY2(out, (inputEnumPortName + " value \"" + json_object_get_string(value) + "\" (\"" + json_object_get_string(name) + "\") failed to produce an output image").c_str());
			if (out)
			{
				VuoImage outputImage = VuoImage_makeFromJson(out);
				QVERIFY(outputImage);
				VuoLocal(outputImage);
			}
		}

		runner->stop();
		delete runner;
	}
};

int main(int argc, char *argv[])
{
	TestImageFilters tc(argc, argv);
	return QTest::qExec(&tc, argc, argv);
}

#include "TestImageFilters.moc"
