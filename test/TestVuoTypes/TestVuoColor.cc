/**
 * @file
 * TestVuoColor implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoColor.h"
#include "VuoColorspace.h"
#include "VuoList_VuoColor.h"
}

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoColor);
Q_DECLARE_METATYPE(VuoList_VuoColor);

/**
 * Tests the VuoColor type.
 */
class TestVuoColor : public QObject
{
	Q_OBJECT

private slots:

	void testStringConversion_data()
	{
		QTest::addColumn<QString>("initializer");
		QTest::addColumn<VuoColor>("value");
		QTest::addColumn<bool>("testTypeValueAsString");

		VuoColor opaqueBlack = VuoColor_makeWithRGBA(0,0,0,1);
		QTest::newRow("emptystring")            << ""                                                           << opaqueBlack      << false;

		VuoColor transparentBlack = VuoColor_makeWithRGBA(0,0,0,0);
		QTest::newRow("transparent black")      << QUOTE({"r":0.0,"g":0.0,"b":0.0,"a":0.0})                     << transparentBlack << true;
		QTest::newRow("transparent black array")<< QUOTE([0,0,0,0])                                             << transparentBlack << false;
		QTest::newRow("transparent black text") << QUOTE("0,0,0,0")                                             << transparentBlack << false;
		QTest::newRow("transparent black hex4") << QUOTE("#0000")                                               << transparentBlack << false;
		QTest::newRow("transparent black hex8") << QUOTE("#00000000")                                           << transparentBlack << false;

		VuoColor opaqueWhite = VuoColor_makeWithRGBA(1,1,1,1);
		QTest::newRow("opaque white")           << QUOTE({"r":1.0,"g":1.0,"b":1.0,"a":1.0})                     << opaqueWhite      << true;
		QTest::newRow("opaque white array")     << QUOTE([1,1,1])                                               << opaqueWhite      << false;
		QTest::newRow("opaque white text3")     << QUOTE("1,1,1")                                               << opaqueWhite      << false;
		QTest::newRow("opaque white text4")     << QUOTE("1,1,1,1")                                             << opaqueWhite      << false;
		QTest::newRow("opaque white hex3")      << QUOTE("#fff")                                                << opaqueWhite      << false;
		QTest::newRow("opaque white hex4")      << QUOTE("#ffff")                                               << opaqueWhite      << false;
		QTest::newRow("opaque white hex6")      << QUOTE("#ffffff")                                             << opaqueWhite      << false;
		QTest::newRow("opaque white hex8")      << QUOTE("#ffffffff")                                           << opaqueWhite      << false;

		VuoColor transparentWhite = VuoColor_makeWithRGBA(1,1,1,0);
		QTest::newRow("invalid white hex8")     << QUOTE("#ffffffgg")                                           << transparentWhite << false;

		VuoColor aquamarine = VuoColor_makeWithRGBA(.5, 1, .75, .5);
		QTest::newRow("aquamarine")             << QUOTE({"r":0.5,"g":1.0,"b":0.75,"a":0.5})                    << aquamarine       << true;
		QTest::newRow("aquamarine text")        << QUOTE(".5,1,.75,.5")											<< aquamarine       << false;
	}
	void testStringConversion()
	{
		QFETCH(QString, initializer);
		QFETCH(VuoColor, value);
		QFETCH(bool, testTypeValueAsString);

		VuoColor actualValue = VuoMakeRetainedFromString(initializer.toUtf8().constData(), VuoColor);
		QVERIFY(VuoColor_areEqual(actualValue, value));
		if (testTypeValueAsString)
			QCOMPARE(VuoColor_getString(value), initializer.toUtf8().constData());
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

	void testConversionBetweenRGBAAndCMYKA_data()
	{
		QTest::addColumn<VuoReal>("r");
		QTest::addColumn<VuoReal>("g");
		QTest::addColumn<VuoReal>("b");
		QTest::addColumn<VuoReal>("a");
		QTest::addColumn<VuoReal>("c");
		QTest::addColumn<VuoReal>("m");
		QTest::addColumn<VuoReal>("y");
		QTest::addColumn<VuoReal>("k");

		//                                         R            G            B           A      C     M      Y           K
		QTest::newRow("white")					<< 1.        << 1.        << 1.       << 1.  << 0. << 0.  << 0.       << 0.      ;
		QTest::newRow("black")					<< 0.        << 0.        << 0.       << 1.  << 0. << 0.  << 0.       << 1.      ;
		QTest::newRow("semi-transparent gray")	<< 0.5       << 0.5       << 0.5      << 0.5 << 0. << 0.  << 0.       << 0.5     ;
		QTest::newRow("red")					<< 1.        << 0.        << 0.       << 1.  << 0. << 1.  << 1.       << 0.      ;
		QTest::newRow("chocolate")				<< 210./255. << 105./255. << 30./255. << 1.  << 0. << 0.5 << 0.857143 << 0.176471;
	}

	void testConversionBetweenRGBAAndCMYKA()
	{
		QFETCH(VuoReal, r);
		QFETCH(VuoReal, g);
		QFETCH(VuoReal, b);
		QFETCH(VuoReal, a);
		QFETCH(VuoReal, c);
		QFETCH(VuoReal, m);
		QFETCH(VuoReal, y);
		QFETCH(VuoReal, k);

		// Test converting CMYKA to RGBA.
		VuoColor colorFromRGBA  = VuoColor_makeWithRGBA(r, g, b, a);
		VuoColor colorFromCMYKA = VuoColorspace_makeCMYKAColor(c, m, y, k, a, 0);
		QVERIFY2(VuoColor_areEqual(colorFromRGBA,colorFromCMYKA), QString("%1 != %2").arg(VuoColor_getHex(colorFromRGBA,true)).arg(VuoColor_getHex(colorFromCMYKA,true)).toUtf8().constData());

		// Test convereting RGBA to CMYKA.
		VuoReal cActual, yActual, mActual, kActual, aActual;
		VuoColorspace_getCMYKA(colorFromRGBA, 0, &cActual, &mActual, &yActual, &kActual, &aActual);
		QVERIFY2(VuoReal_areEqual(c,cActual), QString("%1 != %2").arg(c).arg(cActual).toUtf8().constData());
		QVERIFY2(VuoReal_areEqual(m,mActual), QString("%1 != %2").arg(m).arg(mActual).toUtf8().constData());
		QVERIFY2(VuoReal_areEqual(y,yActual), QString("%1 != %2").arg(y).arg(yActual).toUtf8().constData());
		QVERIFY2(VuoReal_areEqual(k,kActual), QString("%1 != %2").arg(k).arg(kActual).toUtf8().constData());
		QVERIFY2(VuoReal_areEqual(a,aActual), QString("%1 != %2").arg(a).arg(aActual).toUtf8().constData());
	}

	void testAverage_data()
	{
		QTest::addColumn<VuoList_VuoColor>("colorsToAverage");
		QTest::addColumn<VuoColor>("expectedColor");

		{
			VuoList_VuoColor colorsToAverage = VuoListCreate_VuoColor();
			QTest::newRow("no colors") << colorsToAverage << VuoColor_makeWithRGBA(0,0,0,0);
		}

		{
			VuoList_VuoColor colorsToAverage = VuoListCreate_VuoColor();
			VuoListAppendValue_VuoColor(colorsToAverage, VuoColor_makeWithRGBA(0,0,0,1));
			QTest::newRow("black") << colorsToAverage << VuoColor_makeWithRGBA(0,0,0,1);
		}

		{
			VuoList_VuoColor colorsToAverage = VuoListCreate_VuoColor();
			VuoListAppendValue_VuoColor(colorsToAverage, VuoColor_makeWithRGBA(0,0,0,1));
			VuoListAppendValue_VuoColor(colorsToAverage, VuoColor_makeWithRGBA(1,1,1,1));
			QTest::newRow("black and white") << colorsToAverage << VuoColor_makeWithRGBA(.5,.5,.5,1);
		}

		{
			VuoList_VuoColor colorsToAverage = VuoListCreate_VuoColor();
			VuoListAppendValue_VuoColor(colorsToAverage, VuoColor_makeWithRGBA(0,0,0,1));
			VuoListAppendValue_VuoColor(colorsToAverage, VuoColor_makeWithRGBA(1,1,1,0));
			QTest::newRow("black and transparent white") << colorsToAverage << VuoColor_makeWithRGBA(0,0,0,.5);
		}

		{
			VuoList_VuoColor colorsToAverage = VuoListCreate_VuoColor();
			VuoListAppendValue_VuoColor(colorsToAverage, VuoColor_makeWithRGBA(1,0,0,1));
			VuoListAppendValue_VuoColor(colorsToAverage, VuoColor_makeWithRGBA(0,.5,1,.25));
			QTest::newRow("red and semitransparent cyan") << colorsToAverage << VuoColor_makeWithRGBA(.8,.1,.2,.625);
		}
	}
	void testAverage()
	{
		QFETCH(VuoList_VuoColor, colorsToAverage);
		QFETCH(VuoColor, expectedColor);

		VuoColor color = VuoColor_average(colorsToAverage);

		VuoReal tolerance = 0.00001;
		QVERIFY2(fabs(expectedColor.r - color.r) < tolerance, QString("%1 != %2").arg(expectedColor.r).arg(color.r).toUtf8().constData());
		QVERIFY2(fabs(expectedColor.g - color.g) < tolerance, QString("%1 != %2").arg(expectedColor.g).arg(color.g).toUtf8().constData());
		QVERIFY2(fabs(expectedColor.b - color.b) < tolerance, QString("%1 != %2").arg(expectedColor.b).arg(color.b).toUtf8().constData());
		QVERIFY2(fabs(expectedColor.a - color.a) < tolerance, QString("%1 != %2").arg(expectedColor.a).arg(color.a).toUtf8().constData());
	}
};

QTEST_APPLESS_MAIN(TestVuoColor)

#include "TestVuoColor.moc"
