/**
 * @file
 * TestImageFilters implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "TestCompositionExecution.hh"
#include <Vuo/Vuo.h>

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoCompilerNodeClass *);
Q_DECLARE_METATYPE(string);

class TestImageFiltersDelegate : public VuoRunnerDelegateAdapter
{
	void lostContactWithComposition(void)
	{
		QFAIL("Composition crashed.");
	}
};

/**
 * Tests each image filter node for common mistakes.
 */
class TestImageFilters : public TestCompositionExecution
{
	Q_OBJECT

private:
	int argc;
	char **argv;
	VuoCompiler *compiler;

public:

	TestImageFilters(int argc, char **argv)
		: argc(argc),
		  argv(argv)
	{
		compiler = initCompiler();
	}

	~TestImageFilters()
	{
		delete compiler;
	}

private slots:

	VuoPortClass *getFirstImagePort(vector<VuoPortClass *> ports)
	{
		for (vector<VuoPortClass *>::iterator p = ports.begin(); p != ports.end(); ++p)
		{
			VuoCompilerPortClass *cpc = dynamic_cast<VuoCompilerPortClass *>((*p)->getCompiler());
			VuoType *dataType = cpc->getDataVuoType();
			if (!dataType)
				continue;
			if (cpc->getDataVuoType()->getModuleKey() == "VuoImage")
				return *p;
		}
		return NULL;
	}

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

		if (argc > 1)
		{
			// If a single test is specified on the command line,
			// improve performance by loading only data for that test.
			QString nodeClassName(QString::fromUtf8(argv[1]).section(':', 1));
			if (!nodeClassName.isEmpty())
			{
				VuoCompilerNodeClass *nc = compiler->getNodeClass(nodeClassName.toStdString());
				VuoPortClass *imageInput  = getFirstImagePort(nc->getBase()->getInputPortClasses());
				VuoPortClass *imageOutput = getFirstImagePort(nc->getBase()->getOutputPortClasses());
				QTest::newRow(nodeClassName.toUtf8().constData()) << nc << imageInput->getName() << imageOutput->getName();
				return;
			}
		}

		map<string, VuoCompilerNodeClass *> allNodeClasses = compiler->getNodeClasses();
		for (map<string, VuoCompilerNodeClass *>::iterator nc = allNodeClasses.begin(); nc != allNodeClasses.end(); ++nc)
		{
			if (!VuoStringUtilities::beginsWith(nc->first, "vuo."))
				continue;

			VuoPortClass *imageInput  = getFirstImagePort(nc->second->getBase()->getInputPortClasses());
			VuoPortClass *imageOutput = getFirstImagePort(nc->second->getBase()->getOutputPortClasses());
			if (!imageInput || !imageOutput)
				continue;

			QTest::newRow(nc->first.c_str()) << nc->second << imageInput->getName() << imageOutput->getName();
		}
	}
	void testImageFollowedByNull()
	{
		QFETCH(VuoCompilerNodeClass *, nodeClass);
		QFETCH(string, inputPort);
		QFETCH(string, outputPort);

//		printf("%s\n", QTest::currentDataTag()); fflush(stdout);
//		printf("%s\n",TestCompositionExecution::wrapNodeInComposition(nodeClass, compiler).c_str());

		VuoCompilerIssues issues;
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(QTest::currentDataTag(), "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(QTest::currentDataTag(), "");
		compiler->compileCompositionString(TestCompositionExecution::wrapNodeInComposition(nodeClass, compiler), compiledCompositionPath, true, &issues);
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_FastBuildExistingCache);
		remove(compiledCompositionPath.c_str());
		VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, ".", false, true);
		QVERIFY(runner);

		TestImageFiltersDelegate delegate;
		runner->setDelegate(&delegate);

		runner->setRuntimeChecking(true);
		runner->start();

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
