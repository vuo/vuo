/**
 * @file
 * TestVuoReal implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoReal.h"
}

// Be able to use this type in QTest::addColumn()
Q_DECLARE_METATYPE(VuoReal);

/**
 * Tests the VuoReal type.
 */
class TestVuoReal : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase()
	{
		VuoHeap_init();
	}

	void testStringConversion_data()
	{
		QTest::addColumn<QString>("initializer");
		QTest::addColumn<VuoReal>("value");
		QTest::addColumn<bool>("testSummary");

		QTest::newRow("zero") << "0" << (VuoReal)0 << true;
		QTest::newRow("positive") << "1" << (VuoReal)1 << true;
		QTest::newRow("negative") << "-1" << (VuoReal)-1 << true;
		QTest::newRow("decimal with leading 0") << "0.5" << (VuoReal)0.5 << true;
		QTest::newRow("decimal without leading 0") << ".5" << (VuoReal)0 << false;  // JSON specification does not allow
		QTest::newRow("double max") << "1.79769e+308" << (VuoReal)1.79769e+308 << true;
		QTest::newRow("double min") << "2.22507e-308" << (VuoReal)2.22507e-308 << true;
		QTest::newRow("unreal") << "Otto von Bismarck" << (VuoReal)0 << false;
	}
	void testStringConversion()
	{
		QFETCH(QString, initializer);
		QFETCH(VuoReal, value);
		QFETCH(bool, testSummary);

		VuoReal t = VuoReal_valueFromString(initializer.toUtf8().constData());
		QCOMPARE(t, value);

		if (testSummary)
			QCOMPARE(VuoReal_summaryFromValue(value), initializer.toUtf8().constData());
	}
};

QTEST_APPLESS_MAIN(TestVuoReal)

#include "TestVuoReal.moc"
