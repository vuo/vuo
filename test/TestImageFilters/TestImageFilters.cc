/**
 * @file
 * TestImageFilters implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
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
};

int main(int argc, char *argv[])
{
	TestImageFilters tc(argc, argv);
	return QTest::qExec(&tc, argc, argv);
}

#include "TestImageFilters.moc"
