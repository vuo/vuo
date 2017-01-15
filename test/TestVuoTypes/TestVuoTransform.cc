/**
 * @file
 * TestVuoTransform implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoTransform.h"
}

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoPoint3d);
Q_DECLARE_METATYPE(VuoPoint4d);
Q_DECLARE_METATYPE(VuoTransform);
Q_DECLARE_METATYPE(float*);

/**
 * Tests the VuoTransform type.
 */
class TestVuoTransform : public QObject
{
	Q_OBJECT

private slots:

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

		{
			VuoPoint3d identityBasis[3] = {VuoPoint3d_make(1,0,0), VuoPoint3d_make(0,1,0), VuoPoint3d_make(0,0,1)};
			QTest::newRow("identity basis") << VuoTransform_quaternionFromBasis(identityBasis) << VuoPoint4d_make(0,0,0,1);

			VuoPoint3d x90Basis[3] = {VuoPoint3d_make(1,0,0), VuoPoint3d_make(0,0,-1), VuoPoint3d_make(0,1,0)};
			QTest::newRow("basis 90° about X") << VuoTransform_quaternionFromBasis(x90Basis) << VuoPoint4d_make(sqrt(.5),0,0,sqrt(.5));
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

	void testTransformPoint_data()
	{
		QTest::addColumn<VuoTransform>("transform");
		QTest::addColumn<VuoPoint3d>("point");
		QTest::addColumn<VuoPoint3d>("expectedPoint");

		QTest::newRow("identity 0") << VuoTransform_makeIdentity() << VuoPoint3d_make(0,0,0) << VuoPoint3d_make(0,0,0);
		QTest::newRow("identity 1") << VuoTransform_makeIdentity() << VuoPoint3d_make(1,1,1) << VuoPoint3d_make(1,1,1);

		{
			VuoTransform t = VuoTransform_makeEuler(VuoPoint3d_make(0,0,0), VuoPoint3d_make(0, M_PI/2., 0), VuoPoint3d_make(1,1,1));
			QTest::newRow("rotate 90° CCW around y axis (euler)") << t << VuoPoint3d_make(1,0,0) << VuoPoint3d_make(0,0,-1);
		}

		{
			VuoTransform t = VuoTransform_makeEuler(VuoPoint3d_make(0,0,0), VuoPoint3d_make(0, M_PI/4., 0), VuoPoint3d_make(1,1,1));
			QTest::newRow("rotate 45° CCW around y axis (euler)") << t << VuoPoint3d_make(1,0,0) << VuoPoint3d_make(cos(M_PI/4),0,-sin(M_PI/4));
		}

		{
			VuoTransform t = VuoTransform_makeEuler(VuoPoint3d_make(1,0,0), VuoPoint3d_make(0, M_PI/2., 0), VuoPoint3d_make(2,2,2));
			QTest::newRow("rotate, scale, translate") << t << VuoPoint3d_make(1,0,0) << VuoPoint3d_make(1,0,-2);
		}

		{
			VuoTransform t = VuoTransform_makeQuaternion(
				VuoPoint3d_make(0,0,0),
				VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(0,1,0), M_PI/4),
				VuoPoint3d_make(1,1,1));
			QTest::newRow("rotate 45° CCW around y axis (VuoTransform_quaternionFromAxisAngle)") << t << VuoPoint3d_make(1,0,0) << VuoPoint3d_make(cos(M_PI/4),0,-sin(M_PI/4));
		}

		{
			VuoTransform t = VuoTransform_makeQuaternion(
				VuoPoint3d_make(0,0,0),
				VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(0,1,0), M_PI/2),
				VuoPoint3d_make(1,1,1));
			QTest::newRow("rotate 90° CCW around y axis (VuoTransform_quaternionFromAxisAngle)") << t << VuoPoint3d_make(1,0,0) << VuoPoint3d_make(0,0,-1);
		}

		{
			VuoTransform t = VuoTransform_makeQuaternion(
				VuoPoint3d_make(0,0,0),
				VuoTransform_quaternionFromVectors(VuoPoint3d_make(0,0,1), VuoPoint3d_make(0,-1,0)),
				VuoPoint3d_make(1,1,1));
			QTest::newRow("rotate 90° CCW around x axis (VuoTransform_quaternionFromVectors)") << t << VuoPoint3d_make(0,0,1) << VuoPoint3d_make(0,-1,0);
		}

		{
			VuoTransform t = VuoTransform_makeQuaternion(
				VuoPoint3d_make(0,0,0),
				VuoTransform_quaternionFromVectors(VuoPoint3d_make(1,0,0), VuoPoint3d_make(cos(M_PI/4),0,-sin(M_PI/4))),
				VuoPoint3d_make(1,1,1));
			QTest::newRow("rotate 45° CCW around y axis (VuoTransform_quaternionFromVectors)") << t << VuoPoint3d_make(1,0,0) << VuoPoint3d_make(cos(M_PI/4),0,-sin(M_PI/4));
		}

		{
			VuoTransform t = VuoTransform_makeQuaternion(
				VuoPoint3d_make(0,0,0),
				VuoTransform_quaternionFromVectors(VuoPoint3d_make(1,0,0), VuoPoint3d_make(0,0,-1)),
				VuoPoint3d_make(1,1,1));
			QTest::newRow("rotate 90° CCW around y axis (VuoTransform_quaternionFromVectors)") << t << VuoPoint3d_make(1,0,0) << VuoPoint3d_make(0,0,-1);
		}

		{
			VuoTransform t = VuoTransform_makeQuaternion(
				VuoPoint3d_make(0,0,0),
				VuoTransform_quaternionFromVectors(VuoPoint3d_make(1,0,0), VuoPoint3d_make(0,1,0)),
				VuoPoint3d_make(1,1,1));
			QTest::newRow("rotate 90° CCW around z axis (VuoTransform_quaternionFromVectors)") << t << VuoPoint3d_make(1,0,0) << VuoPoint3d_make(0,1,0);
		}

		{
			VuoTransform t = VuoTransform_makeFromTarget(VuoPoint3d_make(0,0,0),VuoPoint3d_make(0,0,-1),VuoPoint3d_make(0,1,0));
			QTest::newRow("identity 0 (VuoTransform_makeFromTarget)") << t << VuoPoint3d_make(1,0,0) << VuoPoint3d_make(1,0,0);
		}

		{
			VuoTransform t = VuoTransform_makeFromTarget(VuoPoint3d_make(0,0,0),VuoPoint3d_make(0,0,-2),VuoPoint3d_make(0,1,0));
			QTest::newRow("identity 1 (VuoTransform_makeFromTarget)") << t << VuoPoint3d_make(1,0,0) << VuoPoint3d_make(1,0,0);
		}

		{
			VuoTransform t = VuoTransform_makeFromTarget(VuoPoint3d_make(0,0,0),VuoPoint3d_make(-1,0,0),VuoPoint3d_make(0,1,0));
			QTest::newRow("rotate 90° CCW around y axis (VuoTransform_makeFromTarget)") << t << VuoPoint3d_make(1,0,0) << VuoPoint3d_make(0,0,-1);
		}
	}
	void testTransformPoint()
	{
		QFETCH(VuoTransform, transform);
		QFETCH(VuoPoint3d, point);
		QFETCH(VuoPoint3d, expectedPoint);

		float matrix[16];
		VuoTransform_getMatrix(transform, matrix);
		VuoPoint3d actualPoint = VuoTransform_transformPoint(matrix, point);

		// "In the case of comparing floats and doubles, qFuzzyCompare() is used for comparing. This means that comparing to 0 will likely fail."
		QCOMPARE(actualPoint.x+10.f, expectedPoint.x+10.f);
		QCOMPARE(actualPoint.y+10.f, expectedPoint.y+10.f);
		QCOMPARE(actualPoint.z+10.f, expectedPoint.z+10.f);


		// Also use this data to test VuoTransform_makeFromMatrix4x4().
		{
			float inputMatrix[16];
			VuoTransform_getMatrix(transform, inputMatrix);
			VuoTransform outputTransform = VuoTransform_makeFromMatrix4x4(inputMatrix);

			float outputMatrix[16];
			VuoTransform_getMatrix(outputTransform, outputMatrix);

			for (int i = 0; i < 16; ++i)
				QCOMPARE(inputMatrix[i]+10.f, outputMatrix[i]+10.f);
		}
	}

	void testDirection_data()
	{
		QTest::addColumn<VuoTransform>("transform");
		QTest::addColumn<VuoPoint3d>("expectedDirection");
		QTest::addColumn<VuoPoint3d>("expectedEuler");
		QTest::addColumn<VuoPoint4d>("expectedQuaternion");

		QTest::newRow("identity") << VuoTransform_makeIdentity() << VuoPoint3d_make(1,0,0) << VuoPoint3d_make(0,0,0) << VuoPoint4d_make(0,0,0,1);

		{
			VuoPoint3d euler = VuoPoint3d_make(0, M_PI/2., 0);
			VuoTransform t = VuoTransform_makeEuler(VuoPoint3d_make(0,0,0), euler, VuoPoint3d_make(1,1,1));
			QTest::newRow("rotate 90° around y axis (euler)") << t << VuoPoint3d_make(0,0,-1) << euler << VuoPoint4d_make(0,sqrt(.5),0,sqrt(.5));
		}

		{
			VuoPoint3d euler = VuoPoint3d_make(0, M_PI/2., 0);
			VuoTransform t = VuoTransform_makeEuler(VuoPoint3d_make(1,2,3), euler, VuoPoint3d_make(4,5,6));
			QTest::newRow("rotate 90° around y axis (euler), scale, transform") << t << VuoPoint3d_make(0,0,-1) << euler << VuoPoint4d_make(0,sqrt(.5),0,sqrt(.5));
		}

		{
			VuoPoint4d quaternion = VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(0,1,0), M_PI/2.);
			VuoTransform t = VuoTransform_makeQuaternion(VuoPoint3d_make(0,0,0), quaternion, VuoPoint3d_make(1,1,1));
			QTest::newRow("rotate 90° around y axis (quaternion)") << t << VuoPoint3d_make(0,0,-1) << VuoPoint3d_make(0, M_PI/2., 0) << quaternion;
		}

		{
			VuoPoint4d quaternion = VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(0,1,0), 91. * M_PI/180.);
			VuoTransform t = VuoTransform_makeQuaternion(VuoPoint3d_make(0,0,0), quaternion, VuoPoint3d_make(1,1,1));
			QTest::newRow("rotate 91° around y axis (quaternion)") << t << VuoPoint3d_make(-.01745, 0, -.99985) << VuoPoint3d_make(3.1416, 1.5533, 3.1416) << quaternion;
		}

		{
			VuoPoint4d quaternion = VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(0,1,0), M_PI/2.);
			VuoTransform t = VuoTransform_makeQuaternion(VuoPoint3d_make(1,2,3), quaternion, VuoPoint3d_make(4,5,6));
			QTest::newRow("rotate 90° around y axis (quaternion), scale, transform") << t << VuoPoint3d_make(0,0,-1) << VuoPoint3d_make(0, M_PI/2., 0) << quaternion;
		}

		{
			VuoPoint3d lightPosition = VuoPoint3d_make(0,0,1);
			VuoPoint3d lightTarget = VuoPoint3d_make(0,0,0);
			VuoPoint4d quaternion = VuoTransform_quaternionFromVectors(VuoPoint3d_make(1,0,0), VuoPoint3d_subtract(lightTarget, lightPosition));
			VuoTransform t = VuoTransform_makeQuaternion(lightPosition, quaternion, VuoPoint3d_make(1,1,1));
			QTest::newRow("targeted spotlight direction vector: forward") << t << VuoPoint3d_make(0,0,-1) << VuoPoint3d_make(0, M_PI/2., 0) << quaternion;
		}

		{
			VuoPoint3d lightPosition = VuoPoint3d_make(-1,0,1);
			VuoPoint3d lightTarget = VuoPoint3d_make(0,0,0);
			VuoPoint4d quaternion = VuoTransform_quaternionFromVectors(VuoPoint3d_make(1,0,0), VuoPoint3d_subtract(lightTarget, lightPosition));
			VuoTransform t = VuoTransform_makeQuaternion(lightPosition, quaternion, VuoPoint3d_make(1,1,1));
			QTest::newRow("targeted spotlight direction vector: forward/right") << t << VuoPoint3d_make(sin(M_PI/4),0,-sin(M_PI/4)) << VuoPoint3d_make(0, M_PI/4., 0) << quaternion;
		}
	}
	void testDirection()
	{
		QFETCH(VuoTransform, transform);
		QFETCH(VuoPoint3d, expectedDirection);
		QFETCH(VuoPoint3d, expectedEuler);
		QFETCH(VuoPoint4d, expectedQuaternion);

		VuoPoint3d actualDirection = VuoTransform_getDirection(transform);

		// "In the case of comparing floats and doubles, qFuzzyCompare() is used for comparing. This means that comparing to 0 will likely fail."
		QCOMPARE(actualDirection.x+10.f, expectedDirection.x+10.f);
		QCOMPARE(actualDirection.y+10.f, expectedDirection.y+10.f);
		QCOMPARE(actualDirection.z+10.f, expectedDirection.z+10.f);

		VuoPoint3d actualEuler = VuoTransform_getEuler(transform);
		QCOMPARE(actualEuler.x+10.f, expectedEuler.x+10.f);
		QCOMPARE(actualEuler.y+10.f, expectedEuler.y+10.f);
		QCOMPARE(actualEuler.z+10.f, expectedEuler.z+10.f);

		VuoPoint4d actualQuaternion = VuoTransform_getQuaternion(transform);
		QCOMPARE(actualQuaternion.w+10.f, expectedQuaternion.w+10.f);
		QCOMPARE(actualQuaternion.x+10.f, expectedQuaternion.x+10.f);
		QCOMPARE(actualQuaternion.y+10.f, expectedQuaternion.y+10.f);
		QCOMPARE(actualQuaternion.z+10.f, expectedQuaternion.z+10.f);

		{
			VuoPoint3d initialDirection = (VuoPoint3d){1,0,0};
			VuoPoint3d actualDirection = VuoTransform_rotateVectorWithQuaternion(initialDirection, expectedQuaternion);
			QCOMPARE(actualDirection.x+10.f, expectedDirection.x+10.f);
			QCOMPARE(actualDirection.y+10.f, expectedDirection.y+10.f);
			QCOMPARE(actualDirection.z+10.f, expectedDirection.z+10.f);
		}
	}

	void testMatrixRoundtrip_data()
	{
		QTest::addColumn<VuoTransform>("expectedTransform");
		QTest::addColumn<float *>("expectedMatrix");

		{
			VuoTransform t = VuoTransform_makeIdentity();

			float *matrix = new float[16];
			memset(matrix, 0, sizeof(float)*16);
			matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1;

			QTest::newRow("identity") << t << matrix;
		}

		{
			VuoTransform t = VuoTransform_makeQuaternion(VuoPoint3d_make(1,0,0), VuoTransform_quaternionFromAxisAngle(VuoPoint3d_make(0,1,0), M_PI/2.), VuoPoint3d_make(2,2,2));

			float *matrix = new float[16];
			memset(matrix, 0, sizeof(float)*16);

			matrix[ 0] = 0;
			matrix[ 1] = 0;
			matrix[ 2] = -2;
			matrix[ 3] = 0;

			matrix[ 4] = 0;
			matrix[ 5] = 2;
			matrix[ 6] = 0;
			matrix[ 7] = 0;

			matrix[ 8] = 2;
			matrix[ 9] = 0;
			matrix[10] = 0;
			matrix[11] = 0;

			matrix[12] = 1;
			matrix[13] = 0;
			matrix[14] = 0;
			matrix[15] = 1;

			QTest::newRow("rotate, scale, translate (quaternion)") << t << matrix;
		}
	}
	void testMatrixRoundtrip()
	{
		QFETCH(VuoTransform, expectedTransform);
		QFETCH(float *, expectedMatrix);

		float actualMatrix[16];
		VuoTransform_getMatrix(expectedTransform, actualMatrix);
		for (int i = 0; i < 16; ++i)
			QCOMPARE(actualMatrix[i] + 10.f, expectedMatrix[i] + 10.f);

		VuoTransform actualTransform = VuoTransform_makeFromMatrix4x4(expectedMatrix);
		QEXPECT_FAIL("rotate, scale, translate (quaternion)", "json-c now encodes double-precision, but quaternion is only accurate to float-precision", Continue);
		QCOMPARE(VuoTransform_getString(actualTransform), VuoTransform_getString(expectedTransform));
	}

	void testSerializationAndSummary_data()
	{
		QTest::addColumn<QString>("value");
		QTest::addColumn<bool>("testReverse");
		QTest::addColumn<QString>("summary");

		QTest::newRow("emptystring")	<< ""
										<< false
										<< "identity transform (no change)";

		QTest::newRow("Euler identity")	<< (const char*)VuoTransform_getString(VuoTransform_makeEuler(VuoPoint3d_make(0,0,0),VuoPoint3d_make(0,0,0),VuoPoint3d_make(1,1,1)))
										<< true
										<< "identity transform (no change)";

		QTest::newRow("Quaternion identity")	<< (const char*)VuoTransform_getString(VuoTransform_makeQuaternion(VuoPoint3d_make(0,0,0),VuoPoint4d_make(0,0,0,1),VuoPoint3d_make(1,1,1)))
												<< true
												<< "identity transform (no change)";

		QTest::newRow("Targeted identity")	<< (const char*)VuoTransform_getString(VuoTransform_makeFromTarget(VuoPoint3d_make(0,0,0),VuoPoint3d_make(1,0,0),VuoPoint3d_make(0,1,0)))
											<< true
											<< "identity transform (no change)";

		QTest::newRow("Euler transform")	<< (const char*)VuoTransform_getString(VuoTransform_makeEuler(VuoPoint3d_make(1,1,1),VuoPoint3d_make(0,M_PI/2.,2.*M_PI),VuoPoint3d_make(2,2,2)))
											<< true
											<< "translation (1, 1, 1)<br>rotation (0°, 90°, 360°) euler<br>scale (2, 2, 2)";

		QTest::newRow("Quaternion transform")	<< (const char*)VuoTransform_getString(VuoTransform_makeQuaternion(VuoPoint3d_make(1,1,1),VuoPoint4d_make(.5,.5,.5,.5),VuoPoint3d_make(2,2,2)))
												<< true
												<< "translation (1, 1, 1)<br>rotation (0.5, 0.5, 0.5, 0.5) quaternion<br>scale (2, 2, 2)";

		QTest::newRow("Targeted transform")	<< (const char*)VuoTransform_getString(VuoTransform_makeFromTarget(VuoPoint3d_make(1,2,3),VuoPoint3d_make(4,5,6),VuoPoint3d_make(0,1,0)))
											<< true
											<< "position (1, 2, 3)<br>target (4, 5, 6)<br>up (0, 1, 0)";
	}
	void testSerializationAndSummary()
	{
		QFETCH(QString, value);
		QFETCH(bool, testReverse);
		QFETCH(QString, summary);

		VuoTransform t = VuoTransform_makeFromString(value.toUtf8().data());
		if (testReverse)
			QCOMPARE(QString::fromUtf8(VuoTransform_getString(t)), value);
		QCOMPARE(QString::fromUtf8(VuoTransform_getSummary(t)), summary);
	}
};

QTEST_APPLESS_MAIN(TestVuoTransform)

#include "TestVuoTransform.moc"
