/**
 * @file
 * TestVuoColor implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoColor.h"
}

/**
 * Tests the VuoColor type.
 */
class TestVuoColor : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase()
	{
		VuoHeap_init();
	}

	void testConversionBetweenRGBAndHSL_data()
	{
		QTest::addColumn<VuoReal>("rExpected");
		QTest::addColumn<VuoReal>("gExpected");
		QTest::addColumn<VuoReal>("bExpected");
		QTest::addColumn<VuoReal>("aExpected");

		QTest::newRow("white") << 1. << 1. << 1. << 1.;
		QTest::newRow("black") << 0. << 0. << 0. << 1.;
		QTest::newRow("semi-transparent gray") << 0.5 << 0.5 << 0.5 << 0.5;
		QTest::newRow("red") << 1. << 0. << 0. << 1.;
		QTest::newRow("chocolate") << 210./255. << 105./255. << 30./255. << 1.;
	}
	void testConversionBetweenRGBAndHSL()
	{
		QFETCH(VuoReal, rExpected);
		QFETCH(VuoReal, gExpected);
		QFETCH(VuoReal, bExpected);
		QFETCH(VuoReal, aExpected);

		VuoColor colorFromRGBA = VuoColor_makeWithRGBA(rExpected, gExpected, bExpected, aExpected);
		VuoReal h, s, l, aHSLA;
		VuoColor_getHSLA(colorFromRGBA, &h, &s, &l, &aHSLA);
		VuoColor colorFromHSLA = VuoColor_makeWithHSLA(h, s, l, aHSLA);
		VuoReal rActual, gActual, bActual, aActual;
		VuoColor_getRGBA(colorFromHSLA, &rActual, &gActual, &bActual, &aActual);

		VuoReal tolerance = 0.001;
		QVERIFY2(fabs(rExpected - rActual) < tolerance, QString("%1 != %2").arg(rExpected).arg(rActual).toUtf8().constData());
		QVERIFY2(fabs(gExpected - gActual) < tolerance, QString("%1 != %2").arg(gExpected).arg(gActual).toUtf8().constData());
		QVERIFY2(fabs(bExpected - bActual) < tolerance, QString("%1 != %2").arg(bExpected).arg(bActual).toUtf8().constData());
		QCOMPARE(aActual, aExpected);
	}
};

QTEST_APPLESS_MAIN(TestVuoColor)

#include "TestVuoColor.moc"
