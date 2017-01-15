/**
 * @file
 * TestVuoProtocol implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <QtTest/QtTest>

#include "VuoProtocol.hh"
#include "VuoFileUtilities.hh"

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

		QTest::newRow("ImageFilter.vuo")	<< VuoProtocol::getProtocol(VuoProtocol::imageFilter)		<< true;
		QTest::newRow("ImageFilter.vuo")	<< VuoProtocol::getProtocol(VuoProtocol::imageGenerator)	<< false;

		QTest::newRow("ImageGenerator.vuo")	<< VuoProtocol::getProtocol(VuoProtocol::imageFilter)		<< false;
		QTest::newRow("ImageGenerator.vuo")	<< VuoProtocol::getProtocol(VuoProtocol::imageGenerator)	<< true;

		QTest::newRow("NoProtocol.vuo")		<< VuoProtocol::getProtocol(VuoProtocol::imageFilter)		<< false;
		QTest::newRow("NoProtocol.vuo")		<< VuoProtocol::getProtocol(VuoProtocol::imageGenerator)	<< false;
	}
	void testComplianceParsing(void)
	{
		QFETCH(VuoProtocol *, protocol);
		QFETCH(bool, expectedCompliance);
		
		string compositionString = VuoFileUtilities::readFileToString(string("composition/") + QTest::currentDataTag());
		QCOMPARE(protocol->isCompositionCompliant(compositionString), expectedCompliance);
	}
};

QTEST_APPLESS_MAIN(TestVuoProtocol)
#include "TestVuoProtocol.moc"
