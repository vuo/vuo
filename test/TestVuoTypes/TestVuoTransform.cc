/**
 * @file
 * TestVuoTransform implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoTransform.h"
}

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoPoint4d);

/**
 * Tests the VuoTransform type.
 */
class TestVuoTransform : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase()
	{
		VuoHeap_init();
	}

	void testQuaternionVuoPoint4d_data()
	{
		QTest::addColumn<VuoPoint4d>("vector1");
		QTest::addColumn<VuoPoint4d>("vector2");

		{
			VuoPoint4d identity = VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(1,0,0),0);
			QTest::newRow("identity")           << identity                                            << VuoPoint4d_make(0,0,0,1);
			QTest::newRow("identity, composed") << VuoTransform_quaternionComposite(identity,identity) << VuoPoint4d_make(0,0,0,1);

			QTest::newRow( "90° about X") << VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(1,0,0), .5*M_PI) << VuoPoint4d_make(sqrt(.5),0,0,sqrt(.5));
			QTest::newRow( "90° about Y") << VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(0,1,0), .5*M_PI) << VuoPoint4d_make(0,sqrt(.5),0,sqrt(.5));
			QTest::newRow( "90° about Z") << VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(0,0,1), .5*M_PI) << VuoPoint4d_make(0,0,sqrt(.5),sqrt(.5));

			QTest::newRow("180° about X") << VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(1,0,0),    M_PI) << VuoPoint4d_make(1,0,0, 0);
			QTest::newRow("180° about Y") << VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(0,1,0),    M_PI) << VuoPoint4d_make(0,1,0, 0);
			QTest::newRow("180° about Z") << VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(0,0,1),    M_PI) << VuoPoint4d_make(0,0,1, 0);

			QTest::newRow("360° about X") << VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(1,0,0), 2.*M_PI) << VuoPoint4d_make(0,0,0,-1);
			QTest::newRow("360° about Y") << VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(0,1,0), 2.*M_PI) << VuoPoint4d_make(0,0,0,-1);
			QTest::newRow("360° about Z") << VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(0,0,1), 2.*M_PI) << VuoPoint4d_make(0,0,0,-1);

			VuoPoint4d x180 = VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(1,0,0),M_PI);
			QTest::newRow("180° about X, composed") << VuoTransform_quaternionComposite(x180,x180) << VuoPoint4d_make(0,0,0,-1);
		}

		{
			VuoTransform identity = VuoTransform_makeQuaternion(VuoPoint3d_make(0,0,0), VuoPoint4d_make(0,0,0,1), VuoPoint3d_make(1,1,1));
			VuoPoint4d m[4];
			VuoTransform_getMatrix(identity, (float *)&m);
			QTest::newRow("identity matrix, 0") << m[0] << VuoPoint4d_make(1,0,0,0);
			QTest::newRow("identity matrix, 1") << m[1] << VuoPoint4d_make(0,1,0,0);
			QTest::newRow("identity matrix, 2") << m[2] << VuoPoint4d_make(0,0,1,0);
			QTest::newRow("identity matrix, 3") << m[3] << VuoPoint4d_make(0,0,0,1);
		}
	}
	void testQuaternionVuoPoint4d()
	{
		QFETCH(VuoPoint4d, vector1);
		QFETCH(VuoPoint4d, vector2);
		// "In the case of comparing floats and doubles, qFuzzyCompare() is used for comparing. This means that comparing to 0 will likely fail."
		QCOMPARE(vector1.x+10.f, vector2.x+10.f);
		QCOMPARE(vector1.y+10.f, vector2.y+10.f);
		QCOMPARE(vector1.z+10.f, vector2.z+10.f);
		QCOMPARE(vector1.w+10.f, vector2.w+10.f);
	}

	void testSerializationAndSummary_data()
	{
		QTest::addColumn<QString>("value");
		QTest::addColumn<bool>("testReverse");
		QTest::addColumn<QString>("summary");

		QTest::newRow("emptystring")	<< ""
										<< false
										<< "identity transform (no change)";

		QTest::newRow("Euler identity")	<< (const char*)VuoTransform_stringFromValue(VuoTransform_makeEuler(VuoPoint3d_make(0,0,0),VuoPoint3d_make(0,0,0),VuoPoint3d_make(1,1,1)))
										<< true
										<< "identity transform (no change)";

		QTest::newRow("Quaternion identity")	<< (const char*)VuoTransform_stringFromValue(VuoTransform_makeQuaternion(VuoPoint3d_make(0,0,0),VuoPoint4d_make(0,0,0,1),VuoPoint3d_make(1,1,1)))
												<< true
												<< "identity transform (no change)";

		QTest::newRow("Euler transform")	<< (const char*)VuoTransform_stringFromValue(VuoTransform_makeEuler(VuoPoint3d_make(1,1,1),VuoPoint3d_make(0,M_PI/2.,2.*M_PI),VuoPoint3d_make(2,2,2)))
											<< true
											<< "translation (1, 1, 1)<br>rotation (0°, 90°, 360°) euler<br>scale (2, 2, 2)";

		QTest::newRow("Quaternion transform")	<< (const char*)VuoTransform_stringFromValue(VuoTransform_makeQuaternion(VuoPoint3d_make(1,1,1),VuoPoint4d_make(.5,.5,.5,.5),VuoPoint3d_make(2,2,2)))
												<< true
												<< "translation (1, 1, 1)<br>rotation (0.5, 0.5, 0.5, 0.5) quaternion<br>scale (2, 2, 2)";
	}
	void testSerializationAndSummary()
	{
		QFETCH(QString, value);
		QFETCH(bool, testReverse);
		QFETCH(QString, summary);

		VuoTransform t = VuoTransform_valueFromString(value.toUtf8().data());
		if (testReverse)
			QCOMPARE(QString::fromUtf8(VuoTransform_stringFromValue(t)), value);
		QCOMPARE(QString::fromUtf8(VuoTransform_summaryFromValue(t)), summary);
	}
};

QTEST_APPLESS_MAIN(TestVuoTransform)

#include "TestVuoTransform.moc"
