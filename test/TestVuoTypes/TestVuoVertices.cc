/**
 * @file
 * TestVuoVertices implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoVertices.h"
}

/**
 * Tests the VuoVertices type.
 */
class TestVuoVertices : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase()
	{
		VuoHeap_init();
	}

	void testSerializationAndSummary_data()
	{
		QTest::addColumn<QString>("value");
//		QTest::addColumn<bool>("valid"); // Is @c value expected to produce a valid result?
		QTest::addColumn<QString>("summary");

		QTest::newRow("emptystring")	<< ""
//										<< false
										<< "0 vertices";

		QTest::newRow("quad")			<< "{\"positions\":[[-0.600000,-0.500000,0.000000,0.000000],[0.500000,-0.500000,0.000000,0.000000],[-0.500000,0.500000,0.000000,0.000000],[0.500000,0.500000,0.000000,0.000000]],\"elementAssemblyMethod\":\"triangleFan\",\"elements\":[0,1,2,3]}"
//										<< false
										<< "4 vertices in a fan of 2 triangles<br>with first position (-0.6, -0.5, 0, 0)";

		QTest::newRow("quadExtended")	<< "{"
										   "\"positions\":[[-0.600000,-0.500000,0.000000,0.000000],[0.500000,-0.500000,0.000000,0.000000],[-0.500000,0.500000,0.000000,0.000000],[0.500000,0.500000,0.000000,0.000000]],"
										   "\"normals\":[[0.000001,0.000002,0.000003,0.000004],[0.000005,0.000006,0.000007,0.000008],[0.000009,0.000010,0.000011,0.000012],[0.000013,0.000014,0.000015,0.000016]],"
										   "\"tangents\":[[0.000101,0.000102,0.000103,0.000104],[0.000105,0.000106,0.000107,0.000108],[0.000109,0.000110,0.000111,0.000112],[0.000113,0.000114,0.000115,0.000116]],"
										   "\"bitangents\":[[0.000201,0.000202,0.000203,0.000204],[0.000205,0.000206,0.000207,0.000208],[0.000209,0.000210,0.000211,0.000212],[0.000213,0.000214,0.000215,0.000216]],"
										   "\"textureCoordinates\":[[0.000301,0.000302,0.000303,0.000304],[0.000305,0.000306,0.000307,0.000308],[0.000309,0.000310,0.000311,0.000312],[0.000313,0.000314,0.000315,0.000316]],"
										   "\"elementAssemblyMethod\":\"triangleStrip\","
										   "\"elements\":[0,1,2,3]"
										   "}"
//										<< true
										<< "4 vertices in a strip of 2 triangles<br>with first position (-0.6, -0.5, 0, 0)";
	}
	void testSerializationAndSummary()
	{
		QFETCH(QString, value);
//		QFETCH(bool, valid);
		QFETCH(QString, summary);

		VuoVertices v = VuoVertices_valueFromString(value.toUtf8().data());
/// @todo reenable, and either test interprocess serialization, or test the reconstituted vertex values (instead of comparing serialized string, since it now contains heap pointers)
//		if (valid)
//			QCOMPARE(QString::fromUtf8(VuoVertices_stringFromValue(v)), value);
		QCOMPARE(QString::fromUtf8(VuoVertices_summaryFromValue(v)), summary);
	}
};

QTEST_APPLESS_MAIN(TestVuoVertices)

#include "TestVuoVertices.moc"
