/**
 * @file
 * TestExport implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#include <QtTest/QtTest>
#pragma clang diagnostic pop

#include "../../framework/Vuo.framework/Headers/Vuo.h" // Use this instead of framework-style Vuo/Vuo.h so Qt Creator can find it.

/**
 * Tests exporting a composition.
 */
class TestExport : public QObject
{
	Q_OBJECT

private slots:

	void initTestCase()
	{
		VuoCompiler c;
		c.loadStoredLicense(false);
	}

	void testMovieExport()
	{
		VuoMovieExporterParameters p;
		VuoMovieExporter e("../../node/vuo.image/examples/GenerateCheckerboardImage.vuo", "GenerateCheckerboardImage.mov", p);
		unsigned int expectedFrameCount = e.getTotalFrameCount();
		QCOMPARE(expectedFrameCount, 300u);

		unsigned int actualFrameCount = 1;
		while (e.exportNextFrame())
			++actualFrameCount;
		QCOMPARE(actualFrameCount, expectedFrameCount);
	}

	void testMovieExportException_data()
	{
		QTest::addColumn<QString>("compositionName");
		QTest::addColumn<int>("spatialSupersampling");
		QTest::addColumn<QString>("expectedException");

		QTest::newRow("Empty composition filename")
				<< ""
				<< 1
				<< "can't parse composition path";

		QTest::newRow("Composition doesn't exist")
				<< "composition/nonexistent-composition.vuo"
				<< 1
				<< "can't open composition";

		QTest::newRow("Composition doesn't even try to output an image")
				<< "composition/no-image.vuo"
				<< 1
				<< "couldn't generate the image; perhaps there is insufficient video memory";

		QTest::newRow("Composition fails to output an image because there isn't enough VRAM")
				<< "composition/no-image-vram.vuo"
				<< 8
				<< "couldn't generate the image; perhaps there is insufficient video memory";

		QTest::newRow("Composition fails to output an image because the image dimensions are larger than OpenGL allows")
				<< "composition/no-image-vram.vuo"
				<< 16
				<< "couldn't generate the image; perhaps there is insufficient video memory";
	}
	void testMovieExportException()
	{
		QFETCH(QString, compositionName);
		QFETCH(int, spatialSupersampling);
		QFETCH(QString, expectedException);

		try
		{
			VuoMovieExporterParameters p(1920, 1080, 0, 1, 5, spatialSupersampling);
			VuoMovieExporter e(compositionName.toStdString(), (compositionName + ".mov").toStdString(), p);
			e.exportNextFrame();
		}
		catch (std::runtime_error &e)
		{
			QCOMPARE(QString(e.what()), expectedException);
			return;
		}
		catch (...)
		{
			QFAIL("Expected an std::runtime_error, but got some other kind of exception.");
		}

		QFAIL("Expected an exception but didn't catch one.");
	}

	void testMovieExportCommandLine()
	{
		int ret = system(("../../framework/vuo-export movie ../../node/vuo.image/examples/GenerateCheckerboardImage.vuo"));
		QCOMPARE(ret, 0);
		QVERIFY(VuoFileUtilities::fileContainsReadableData("../../node/vuo.image/examples/GenerateCheckerboardImage.mov"));
	}

};

QTEST_APPLESS_MAIN(TestExport)
#include "TestExport.moc"
