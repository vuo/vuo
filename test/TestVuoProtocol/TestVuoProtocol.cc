/**
 * @file
 * TestVuoProtocol implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <Vuo/Vuo.h>

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoProtocol *);

/**
 * Tests the @c VuoProtocol class.
 */
class TestVuoProtocol : public QObject
{
	Q_OBJECT

private slots:

	void testComplianceParsing_data()
	{
		QTest::addColumn<VuoProtocol *>("protocol");
		QTest::addColumn<bool>("expectedCompliance");

		QTest::newRow("ImageFilter.vuo filter")         << VuoProtocol::getProtocol(VuoProtocol::imageFilter)       << true;
		QTest::newRow("ImageFilter.vuo generator")      << VuoProtocol::getProtocol(VuoProtocol::imageGenerator)    << false;
		QTest::newRow("ImageFilter.vuo transition")     << VuoProtocol::getProtocol(VuoProtocol::imageTransition)   << false;

		QTest::newRow("ImageGenerator.vuo filter")      << VuoProtocol::getProtocol(VuoProtocol::imageFilter)       << false;
		QTest::newRow("ImageGenerator.vuo generator")   << VuoProtocol::getProtocol(VuoProtocol::imageGenerator)    << true;
		QTest::newRow("ImageGenerator.vuo transition")  << VuoProtocol::getProtocol(VuoProtocol::imageTransition)   << false;

		QTest::newRow("ImageTransition.vuo filter")     << VuoProtocol::getProtocol(VuoProtocol::imageFilter)       << false;
		QTest::newRow("ImageTransition.vuo generator")  << VuoProtocol::getProtocol(VuoProtocol::imageGenerator)    << false;
		QTest::newRow("ImageTransition.vuo transition") << VuoProtocol::getProtocol(VuoProtocol::imageTransition)   << true;

		QTest::newRow("NoProtocol.vuo filter")          << VuoProtocol::getProtocol(VuoProtocol::imageFilter)       << false;
		QTest::newRow("NoProtocol.vuo generator")       << VuoProtocol::getProtocol(VuoProtocol::imageGenerator)    << false;
		QTest::newRow("NoProtocol.vuo transition")      << VuoProtocol::getProtocol(VuoProtocol::imageTransition)   << false;
	}
	void testComplianceParsing(void)
	{
		QFETCH(VuoProtocol *, protocol);
		QFETCH(bool, expectedCompliance);

		string compositionPath = string("composition/") + QTest::currentDataTag();
		compositionPath.erase(compositionPath.find(' '));

		string compositionString = VuoFileUtilities::readFileToString(compositionPath);

		VuoCompiler compiler;
		VuoCompilerIssues issues;
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(QTest::currentDataTag(), "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(QTest::currentDataTag(), "");
		compiler.compileCompositionString(compositionString, compiledCompositionPath, true, &issues);
		compiler.linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_FastBuildExistingCache);
		remove(compiledCompositionPath.c_str());
		VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, ".", false, true);
		QVERIFY(runner);
		runner->setRuntimeChecking(true);
		runner->start();

		// Test isCompositionCompliant(string)
		QCOMPARE(protocol->isCompositionCompliant(compositionString), expectedCompliance);

		// Test isCompositionCompliant(VuoRunner*)
		QCOMPARE(protocol->isCompositionCompliant(runner), expectedCompliance);

		if (expectedCompliance
		 || compositionPath == "composition/NoProtocol.vuo")
		{
			// Test getCompositionProtocols(string)
			vector<VuoProtocol *> protocols = VuoProtocol::getCompositionProtocols(compositionString);
			vector<VuoProtocol *> expectedProtocols;
			if (expectedCompliance)
				expectedProtocols.push_back(protocol);
			QCOMPARE(protocols, expectedProtocols);

			// Test getCompositionProtocols(VuoRunner*)
			vector<VuoProtocol *> protocolsR = VuoProtocol::getCompositionProtocols(compositionString);
			QCOMPARE(protocolsR, expectedProtocols);
		}

		runner->stop();
		delete runner;
	}
};

QTEST_APPLESS_MAIN(TestVuoProtocol)
#include "TestVuoProtocol.moc"
