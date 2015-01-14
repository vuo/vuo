/**
 * @file
 * TestVuoSceneObject implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoSceneObject.h"
#include "VuoList_VuoSceneObject.h"
#include "VuoList_VuoVertices.h"
}

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoSceneObject);

/**
 * Tests the VuoVertices type.
 */
class TestVuoSceneObject : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase()
	{
		VuoHeap_init();
	}

	void testMakeAndSummaryAndSerialization_data()
	{
		QTest::addColumn<VuoSceneObject>("value");
		QTest::addColumn<QString>("summary");
		QTest::addColumn<QString>("json");

		QTest::newRow("emptystring")	<< VuoSceneObject_valueFromString("")
										<< "0 vertices, 0 elements<br><br>(no shader)<br><br>identity transform (no change)<br><br>0 child objects"
										<< "{\"transform\":\"identity\"}";

		{
			VuoList_VuoVertices verticesList = VuoListCreate_VuoVertices();
			VuoListAppendValue_VuoVertices(verticesList, VuoVertices_getQuad());
			VuoSceneObject o = VuoSceneObject_make(
						verticesList,
						VuoShader_makeImageShader(),
						VuoTransform_makeIdentity(),
						VuoListCreate_VuoSceneObject()
					);
			QTest::newRow("quad")		<< o
										<< "4 vertices, 4 elements<br><br>image shader<br>with 0 textures<br><br>identity transform (no change)<br><br>0 child objects"
										<< "";	// Don't test serialization since it includes object pointers.
		}

		{
			VuoGlContext glContext = VuoGlContext_use();

			VuoList_VuoVertices verticesList = VuoListCreate_VuoVertices();
			VuoListAppendValue_VuoVertices(verticesList, VuoVertices_getQuad());

			VuoShader s = VuoShader_makeImageShader();
			VuoImage image = VuoImage_make(42,640,480);
			VuoShader_addTexture(s, glContext, "texture", image);

			VuoSceneObject o = VuoSceneObject_make(
						verticesList,
						s,
						VuoTransform_makeIdentity(),
						VuoListCreate_VuoSceneObject()
					);

			QTest::newRow("quad image")	<< o
										<< "4 vertices, 4 elements<br><br>image shader<br>with 1 texture<br><br>identity transform (no change)<br><br>0 child objects"
										<< "";	// Don't test serialization since it includes object pointers.

			VuoGlContext_disuse(glContext);
		}

		{
			VuoList_VuoVertices verticesList = VuoListCreate_VuoVertices();
			VuoListAppendValue_VuoVertices(verticesList, VuoVertices_getQuad());
			VuoSceneObject o = VuoSceneObject_make(
						verticesList,
						VuoShader_makeImageShader(),
						VuoTransform_makeIdentity(),
						VuoListCreate_VuoSceneObject()
					);

			// Create parent of VuoSceneObject o.
			VuoList_VuoVertices verticesList2 = VuoListCreate_VuoVertices();
			VuoListAppendValue_VuoVertices(verticesList2, VuoVertices_getQuad());
			VuoList_VuoSceneObject o2ChildObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(o2ChildObjects, o);
			VuoSceneObject o2 = VuoSceneObject_make(
						verticesList,
						VuoShader_makeImageShader(),
						VuoTransform_makeIdentity(),
						o2ChildObjects
					);

			QTest::newRow("quad quad")	<< o2
										<< "4 vertices, 4 elements<br><br>image shader<br>with 0 textures<br><br>identity transform (no change)<br><br>1 child object<br>1 descendant<br><br>total, including descendants:<br>8 vertices, 8 elements<br>0 textures"
										<< "";	// Don't test serialization since it includes object pointers.
		}

		{
			VuoList_VuoVertices verticesList = VuoListCreate_VuoVertices();
			VuoListAppendValue_VuoVertices(verticesList, VuoVertices_getQuad());
			VuoSceneObject o = VuoSceneObject_make(
						verticesList,
						VuoShader_makeImageShader(),
						VuoTransform_makeIdentity(),
						VuoListCreate_VuoSceneObject()
					);

			// Create parent of VuoSceneObject o.
			VuoList_VuoVertices verticesList2 = VuoListCreate_VuoVertices();
			VuoListAppendValue_VuoVertices(verticesList2, VuoVertices_getQuad());
			VuoList_VuoSceneObject o2ChildObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(o2ChildObjects, o);
			VuoSceneObject o2 = VuoSceneObject_make(
						verticesList,
						VuoShader_makeImageShader(),
						VuoTransform_makeIdentity(),
						o2ChildObjects
					);

			// Create parent of VuoSceneObject o2.
			VuoList_VuoVertices verticesList3 = VuoListCreate_VuoVertices();
			VuoListAppendValue_VuoVertices(verticesList3, VuoVertices_getQuad());
			VuoList_VuoSceneObject o3ChildObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(o3ChildObjects, o2);
			VuoSceneObject o3 = VuoSceneObject_make(
						verticesList,
						VuoShader_makeImageShader(),
						VuoTransform_makeIdentity(),
						o3ChildObjects
					);

			QTest::newRow("quad quad quad")	<< o3
											<< "4 vertices, 4 elements<br><br>image shader<br>with 0 textures<br><br>identity transform (no change)<br><br>1 child object<br>2 descendants<br><br>total, including descendants:<br>12 vertices, 12 elements<br>0 textures"
											<< "";	// Don't test serialization since it includes object pointers.
		}

		{
			VuoSceneObject o = VuoSceneObject_makePerspectiveCamera(
						"vuocam",
						VuoPoint3d_make(42,43,44),
						VuoPoint3d_multiply(VuoPoint3d_make(1,2,3),M_PI/180.),
						42.0,
						1.0,
						20.0
					);
			QTest::newRow("perspective camera")		<< o
													<< "perspective camera<br>at (42, 43, 44)<br>rotated (1, 2, 3)<br>42° field of view<br>shows objects between depth 1 and 20"
													<< "{\"cameraType\":\"perspective\",\"cameraDistanceMin\":1.000000,\"cameraDistanceMax\":20.000000,\"cameraFieldOfView\":42.000000,\"name\":\"vuocam\",\"transform\":{\"translation\":[42.000000,43.000000,44.000000],\"eulerRotation\":[0.017453,0.034907,0.052360],\"scale\":[1.000000,1.000000,1.000000]}}";
		}

		{
			VuoSceneObject o = VuoSceneObject_makeOrthographicCamera(
						"vuocam ortho",
						VuoPoint3d_make(52,53,54),
						VuoPoint3d_multiply(VuoPoint3d_make(4,5,6),M_PI/180.),
						2.0,
						3.0,
						22.0
					);
			QTest::newRow("orthographic camera")	<< o
													<< "orthographic camera<br>at (52, 53, 54)<br>rotated (4, 5, 6)<br>2 unit width<br>shows objects between depth 3 and 22"
													<< "{\"cameraType\":\"orthographic\",\"cameraDistanceMin\":3.000000,\"cameraDistanceMax\":22.000000,\"cameraWidth\":2.000000,\"name\":\"vuocam ortho\",\"transform\":{\"translation\":[52.000000,53.000000,54.000000],\"eulerRotation\":[0.069813,0.087266,0.104720],\"scale\":[1.000000,1.000000,1.000000]}}";
		}
	}
	void testMakeAndSummaryAndSerialization()
	{
		QFETCH(VuoSceneObject, value);
		QFETCH(QString, summary);
		QFETCH(QString, json);

		QCOMPARE(QString::fromUtf8(VuoSceneObject_summaryFromValue(value)), summary);
		if (json.length())
		{
			QCOMPARE(QString::fromUtf8(VuoSceneObject_stringFromValue(value)), json);
			QCOMPARE(QString::fromUtf8(VuoSceneObject_stringFromValue(VuoSceneObject_valueFromString(json.toUtf8().data()))), json);
		}
	}
};

QTEST_APPLESS_MAIN(TestVuoSceneObject)

#include "TestVuoSceneObject.moc"
