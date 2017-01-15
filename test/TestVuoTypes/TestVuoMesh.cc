/**
 * @file
 * TestVuoMesh implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoMesh.h"
}

// Be able to use this type in QTest::addColumn()
Q_DECLARE_METATYPE(VuoMesh);

/**
 * Tests the VuoMesh type.
 */
class TestVuoMesh : public QObject
{
	Q_OBJECT

private slots:

	void testSerializationAndSummary_data()
	{
		QTest::addColumn<VuoMesh>("value");
//		QTest::addColumn<bool>("valid"); // Is @c value expected to produce a valid result?
		QTest::addColumn<QString>("summary");

		QTest::newRow("empty string")	<< (VuoMesh)NULL
//										<< false
										<< "(empty mesh)";

		QTest::newRow("quad")			<< VuoMesh_makeQuadWithoutNormals()
//										<< false
										<< "Mesh containing: <ul><li>4 vertices, 2 triangles<br>with first position (-0.5, -0.5, 0, 1)</li></ul>";

		QTest::newRow("tri")			<< VuoMesh_makeEquilateralTriangle()
//										<< true
										<< "Mesh containing: <ul><li>3 vertices in a strip of 1 triangle<br>with first position (-2.52368e-08, 0.57735, 0, 1)</li></ul>";
	}
	void testSerializationAndSummary()
	{
		QFETCH(VuoMesh, value);
//		QFETCH(bool, valid);
		QFETCH(QString, summary);

/// @todo reenable, and either test interprocess serialization, or test the reconstituted vertex values (instead of comparing serialized string, since it now contains heap pointers)
//		if (valid)
//			QCOMPARE(QString::fromUtf8(VuoMesh_getString(m)), value);
		QCOMPARE(QString::fromUtf8(VuoMesh_getSummary(value)), summary);
	}
};

QTEST_APPLESS_MAIN(TestVuoMesh)

#include "TestVuoMesh.moc"
