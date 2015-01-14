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

	void testMakeAndSummary_data()
	{
		QTest::addColumn<VuoSceneObject>("value");
		QTest::addColumn<QString>("summary");

		QTest::newRow("emptystring")	<< VuoSceneObject_valueFromString("")
										<< "0 vertices, 0 elements<br><br>(no shader)<br><br>identity transform (no change)<br><br>0 child objects";

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
										<< "4 vertices, 4 elements<br><br>image shader<br>with 0 textures<br><br>identity transform (no change)<br><br>0 child objects";
		}

		{
			VuoList_VuoVertices verticesList = VuoListCreate_VuoVertices();
			VuoListAppendValue_VuoVertices(verticesList, VuoVertices_getQuad());

			VuoShader s = VuoShader_makeImageShader();
			VuoImage image = VuoImage_make(42,640,480);
			VuoShader_addTexture(s, image, "texture");

			VuoSceneObject o = VuoSceneObject_make(
						verticesList,
						s,
						VuoTransform_makeIdentity(),
						VuoListCreate_VuoSceneObject()
					);

			QTest::newRow("quad image")	<< o
										<< "4 vertices, 4 elements<br><br>image shader<br>with 1 texture<br><br>identity transform (no change)<br><br>0 child objects";
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
										<< "4 vertices, 4 elements<br><br>image shader<br>with 0 textures<br><br>identity transform (no change)<br><br>1 child object<br>1 descendant<br><br>total, including descendants:<br>8 vertices, 8 elements<br>0 textures";
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
											<< "4 vertices, 4 elements<br><br>image shader<br>with 0 textures<br><br>identity transform (no change)<br><br>1 child object<br>2 descendants<br><br>total, including descendants:<br>12 vertices, 12 elements<br>0 textures";
		}
	}
	void testMakeAndSummary()
	{
		QFETCH(VuoSceneObject, value);
		QFETCH(QString, summary);

		QCOMPARE(QString::fromUtf8(VuoSceneObject_summaryFromValue(value)), summary);
	}
};

QTEST_APPLESS_MAIN(TestVuoSceneObject)

#include "TestVuoSceneObject.moc"
