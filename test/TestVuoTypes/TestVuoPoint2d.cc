/**
 * @file
 * TestVuoPoint2d implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoPoint2d.h"
}

// Be able to use this type in QTest::addColumn()
Q_DECLARE_METATYPE(VuoPoint2d);
Q_DECLARE_METATYPE(VuoRectangle);

/**
 * Tests the VuoPoint2d type.
 */
class TestVuoPoint2d : public QObject
{
	Q_OBJECT

private slots:

	void testStringConversion_data()
	{
		QTest::addColumn<QString>("initializer");
		QTest::addColumn<VuoPoint2d>("value");
		QTest::addColumn<bool>("testStringFromValue");

		{
			VuoPoint2d p;
			p.x = p.y = 0;
			QTest::newRow("zero") << "{\"x\":0,\"y\":0}" << p << true;
			QTest::newRow("emptystring") << "" << p << false;
			QTest::newRow("partial point 1") << "{\"y\":0}" << p << false;
			QTest::newRow("zero text") << QUOTE("0,0") << p << false;
			QTest::newRow("nonpoint") << "Otto von Bismarck" << p << false;
		}

		{
			VuoPoint2d p;
			p.x = p.y = 1;
			QTest::newRow("one") << "{\"x\":1,\"y\":1}" << p << true;
			QTest::newRow("one text") << QUOTE("1,1") << p << false;
		}

		{
			VuoPoint2d p;
			p.x = -0.5;
			p.y = 0.5;
			QTest::newRow("different values") << "{\"x\":-0.5,\"y\":0.5}" << p << true;
			QTest::newRow("different values text") << QUOTE("-.5, .5") << p << false;
		}

		{
			VuoPoint2d p;
			p.x = 0;
			p.y = -0.999;
			QTest::newRow("partial point 2") << "{\"y\":-0.999}" << p << false;
		}

		{
			VuoPoint2d p;
			p.x = p.y = 3.40282e+38;
			QTest::newRow("float max") << "{\"x\":3.40282e+38, \"y\":3.40282e+38}" << p << false;
		}

		{
			VuoPoint2d p;
			p.x = p.y = 1.17549e-38;
			QTest::newRow("float min") << "{\"x\":1.17549e-38, \"y\":1.17549e-38}" << p << false;
		}
	}
	void testStringConversion()
	{
		QFETCH(QString, initializer);
		QFETCH(VuoPoint2d, value);
		QFETCH(bool, testStringFromValue);

		VuoPoint2d p = VuoPoint2d_makeFromString(initializer.toUtf8().constData());

		QCOMPARE(p.x,value.x);
		QCOMPARE(p.y,value.y);

		if (testStringFromValue)
			QCOMPARE(VuoPoint2d_getString(p), initializer.toUtf8().constData());
	}

	void testRectangleIntersection_data()
	{
		QTest::addColumn<VuoRectangle>("rectangleA");
		QTest::addColumn<VuoRectangle>("rectangleB");
		QTest::addColumn<VuoRectangle>("expectedIntersection");

		QTest::newRow("nonintersecting horizontal AB")          << VuoRectangle_make(0,0,1,1) << VuoRectangle_make(2,0,1,1) << VuoRectangle_make(0,0,0,0);
		QTest::newRow("nonintersecting horizontal AB touching") << VuoRectangle_make(0,0,1,1) << VuoRectangle_make(1,0,1,1) << VuoRectangle_make(0,0,0,0);
		QTest::newRow("nonintersecting horizontal BA")          << VuoRectangle_make(2,0,1,1) << VuoRectangle_make(0,0,1,1) << VuoRectangle_make(0,0,0,0);
		QTest::newRow("nonintersecting horizontal BA touching") << VuoRectangle_make(1,0,1,1) << VuoRectangle_make(0,0,1,1) << VuoRectangle_make(0,0,0,0);
		QTest::newRow("nonintersecting vertical AB")            << VuoRectangle_make(0,0,1,1) << VuoRectangle_make(0,2,1,1) << VuoRectangle_make(0,0,0,0);
		QTest::newRow("nonintersecting vertical AB touching")   << VuoRectangle_make(0,0,1,1) << VuoRectangle_make(0,1,1,1) << VuoRectangle_make(0,0,0,0);
		QTest::newRow("nonintersecting vertical BA")            << VuoRectangle_make(0,2,1,1) << VuoRectangle_make(0,0,1,1) << VuoRectangle_make(0,0,0,0);
		QTest::newRow("nonintersecting vertical BA touching")   << VuoRectangle_make(0,1,1,1) << VuoRectangle_make(0,0,1,1) << VuoRectangle_make(0,0,0,0);

		QTest::newRow("intersecting horizontal AB")             << VuoRectangle_make(0  ,0  ,1,1) << VuoRectangle_make(0.5,0  ,1,1) << VuoRectangle_make(0.25,0   ,0.5,1  );
		QTest::newRow("intersecting horizontal BA")             << VuoRectangle_make(0.5,0  ,1,1) << VuoRectangle_make(0  ,0  ,1,1) << VuoRectangle_make(0.25,0   ,0.5,1  );
		QTest::newRow("intersecting vertical AB")               << VuoRectangle_make(0  ,0  ,1,1) << VuoRectangle_make(0  ,0.5,1,1) << VuoRectangle_make(0   ,0.25,1  ,0.5);
		QTest::newRow("intersecting vertical BA")               << VuoRectangle_make(0  ,0.5,1,1) << VuoRectangle_make(0  ,0  ,1,1) << VuoRectangle_make(0   ,0.25,1  ,0.5);
		QTest::newRow("intersecting diagonal AB")               << VuoRectangle_make(0  ,0  ,1,1) << VuoRectangle_make(0.5,0.5,1,1) << VuoRectangle_make(0.25,0.25,0.5,0.5);
		QTest::newRow("intersecting diagonal BA")               << VuoRectangle_make(0.5,0.5,1,1) << VuoRectangle_make(0  ,0  ,1,1) << VuoRectangle_make(0.25,0.25,0.5,0.5);
	}
	void testRectangleIntersection()
	{
		QFETCH(VuoRectangle, rectangleA);
		QFETCH(VuoRectangle, rectangleB);
		QFETCH(VuoRectangle, expectedIntersection);

		VuoRectangle actualIntersection = VuoPoint2d_rectangleIntersection(rectangleA, rectangleB);

		// "In the case of comparing floats and doubles, qFuzzyCompare() is used for comparing. This means that comparing to 0 will likely fail."
		QCOMPARE(actualIntersection.center.x + 10.f, expectedIntersection.center.x + 10.f);
		QCOMPARE(actualIntersection.center.y + 10.f, expectedIntersection.center.y + 10.f);
		QCOMPARE(actualIntersection.size.x   + 10.f, expectedIntersection.size.x   + 10.f);
		QCOMPARE(actualIntersection.size.y   + 10.f, expectedIntersection.size.y   + 10.f);
	}

	void testRectangleUnion_data()
	{
		QTest::addColumn<VuoRectangle>("rectangleA");
		QTest::addColumn<VuoRectangle>("rectangleB");
		QTest::addColumn<VuoRectangle>("expectedUnion");

		QTest::newRow("A left/below B")		<< VuoRectangle_make(0.5,0.5,1,1) << VuoRectangle_make(1.5,1.5,1,1) << VuoRectangle_make(1,1,2,2);
		QTest::newRow("A right/above B")	<< VuoRectangle_make(1.5,1.5,1,1) << VuoRectangle_make(0.5,0.5,1,1) << VuoRectangle_make(1,1,2,2);
	}
	void testRectangleUnion()
	{
		QFETCH(VuoRectangle, rectangleA);
		QFETCH(VuoRectangle, rectangleB);
		QFETCH(VuoRectangle, expectedUnion);

		VuoRectangle actualUnion = VuoPoint2d_rectangleUnion(rectangleA, rectangleB);

		// "In the case of comparing floats and doubles, qFuzzyCompare() is used for comparing. This means that comparing to 0 will likely fail."
		QCOMPARE(actualUnion.center.x + 10.f, expectedUnion.center.x + 10.f);
		QCOMPARE(actualUnion.center.y + 10.f, expectedUnion.center.y + 10.f);
		QCOMPARE(actualUnion.size.x   + 10.f, expectedUnion.size.x   + 10.f);
		QCOMPARE(actualUnion.size.y   + 10.f, expectedUnion.size.y   + 10.f);
	}
};

QTEST_APPLESS_MAIN(TestVuoPoint2d)

#include "TestVuoPoint2d.moc"

