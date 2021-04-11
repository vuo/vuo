/**
 * @file
 * TestVuoRectangle implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoRectangle.h"
}

// Be able to use this type in QTest::addColumn()
Q_DECLARE_METATYPE(VuoRectangle);

/**
 * Tests the VuoRectangle type.
 */
class TestVuoRectangle : public QObject
{
	Q_OBJECT

private slots:

	void testStringConversion_data()
	{
		QTest::addColumn<QString>("initializer");
		QTest::addColumn<VuoRectangle>("value");
		QTest::addColumn<bool>("testStringFromValue");

		QTest::newRow("emptystring") << ""                                                                  << VuoRectangle_make(0,0,0,0) << false;
		QTest::newRow("zero")        << "{\"center\":{\"x\":0.0,\"y\":0.0},\"size\":{\"x\":0.0,\"y\":0.0}}" << VuoRectangle_make(0,0,0,0) << true;
		QTest::newRow("nonzero")     << "{\"center\":{\"x\":1.0,\"y\":2.0},\"size\":{\"x\":3.0,\"y\":4.0}}" << VuoRectangle_make(1,2,3,4) << true;
	}
	void testStringConversion()
	{
		QFETCH(QString, initializer);
		QFETCH(VuoRectangle, value);
		QFETCH(bool, testStringFromValue);

		VuoRectangle r = VuoRectangle_makeFromString(initializer.toUtf8().constData());

		QCOMPARE(r.center.x, value.center.x);
		QCOMPARE(r.center.y, value.center.y);
		QCOMPARE(r.size.x,   value.size.x);
		QCOMPARE(r.size.y,   value.size.y);

		if (testStringFromValue)
			QCOMPARE(VuoRectangle_getString(r), initializer.toUtf8().constData());
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

		VuoRectangle actualIntersection = VuoRectangle_intersection(rectangleA, rectangleB);

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

		QTest::newRow("A left/below B")  << VuoRectangle_make(0.5,0.5,1,1) << VuoRectangle_make(1.5,1.5,1,1) << VuoRectangle_make(1,1,2,2);
		QTest::newRow("A right/above B") << VuoRectangle_make(1.5,1.5,1,1) << VuoRectangle_make(0.5,0.5,1,1) << VuoRectangle_make(1,1,2,2);
	}
	void testRectangleUnion()
	{
		QFETCH(VuoRectangle, rectangleA);
		QFETCH(VuoRectangle, rectangleB);
		QFETCH(VuoRectangle, expectedUnion);

		VuoRectangle actualUnion = VuoRectangle_union(rectangleA, rectangleB);

		// "In the case of comparing floats and doubles, qFuzzyCompare() is used for comparing. This means that comparing to 0 will likely fail."
		QCOMPARE(actualUnion.center.x + 10.f, expectedUnion.center.x + 10.f);
		QCOMPARE(actualUnion.center.y + 10.f, expectedUnion.center.y + 10.f);
		QCOMPARE(actualUnion.size.x   + 10.f, expectedUnion.size.x   + 10.f);
		QCOMPARE(actualUnion.size.y   + 10.f, expectedUnion.size.y   + 10.f);
	}
};

QTEST_APPLESS_MAIN(TestVuoRectangle)

#include "TestVuoRectangle.moc"
