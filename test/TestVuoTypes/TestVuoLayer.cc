/**
 * @file
 * TestVuoLayer implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoLayer.h"
#include "VuoList_VuoLayer.h"
}

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoPoint2d);
Q_DECLARE_METATYPE(VuoLayer);

/**
 * Tests the VuoLayer type.
 */
class TestVuoLayer : public QObject
{
	Q_OBJECT

private slots:

	void testBounds_data()
	{
		QTest::addColumn<VuoLayer>("layer");
		QTest::addColumn<VuoPoint2d>("expectedCenter");
		QTest::addColumn<VuoPoint2d>("expectedSize");
		QTest::addColumn<VuoPoint2d>("expectedCenter3D");
		QTest::addColumn<VuoPoint2d>("expectedSize3D");

		QTest::newRow("empty scene")	<< VuoLayer_makeEmpty()
										<< VuoPoint2d_make(0,0) << VuoPoint2d_make(0,0)
										<< VuoPoint2d_make(0,0) << VuoPoint2d_make(0,0);

		{
			VuoList_VuoLayer childLayers = VuoListCreate_VuoLayer();
			VuoListAppendValue_VuoLayer(childLayers, VuoLayer_make(VuoText_make("child1"), VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1), 1,1), VuoPoint2d_make(-1,0), 0, 1, 1));
			VuoListAppendValue_VuoLayer(childLayers, VuoLayer_make(VuoText_make("child2"), VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1), 1,1), VuoPoint2d_make( 0,1), 0, 1, 1));

			VuoLayer parent = VuoLayer_makeGroup(childLayers, VuoTransform2d_makeIdentity());
			QTest::newRow("two scaled layers")	<< parent
												<< VuoPoint2d_make(-.5,.5) << VuoPoint2d_make(2,2)
												<< VuoPoint2d_make(-.5,.5) << VuoPoint2d_make(2,2);
		}

		{
			VuoList_VuoLayer childLayers = VuoListCreate_VuoLayer();
			VuoListAppendValue_VuoLayer(childLayers, VuoLayer_makeRealSize("child1", VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1), 1,1), VuoPoint2d_make(-1,0), 1));
			VuoListAppendValue_VuoLayer(childLayers, VuoLayer_makeRealSize("child2", VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1), 1,1), VuoPoint2d_make( 0,1), 1));

			VuoLayer parent = VuoLayer_makeGroup(childLayers, VuoTransform2d_makeIdentity());
			QTest::newRow("two real size layers")	<< parent
													<< VuoPoint2d_make(-.4,.4) << VuoPoint2d_make(1.2,1.2)
													<< VuoPoint2d_make(-.5,.5) << VuoPoint2d_make(1,1);
		}
	}
	void testBounds()
	{
		QFETCH(VuoLayer, layer);
		QFETCH(VuoPoint2d, expectedCenter);
		QFETCH(VuoPoint2d, expectedSize);
		QFETCH(VuoPoint2d, expectedCenter3D);
		QFETCH(VuoPoint2d, expectedSize3D);

		VuoRectangle bounds = VuoLayer_getBoundingRectangle(layer, 10, 10, 1);
//		VLog("bounds2d center = %s		size=%s",VuoPoint2d_getSummary(bounds.center),VuoPoint2d_getSummary(bounds.size));

		QCOMPARE(bounds.center.x + 10, expectedCenter.x + 10);
		QCOMPARE(bounds.center.y + 10, expectedCenter.y + 10);
		QCOMPARE(bounds.size.x   + 10, expectedSize.x   + 10);
		QCOMPARE(bounds.size.y   + 10, expectedSize.y   + 10);


		// Also test VuoSceneObject_bounds(), which is used by vuo.layer.combine.center.
		{
			VuoBox bounds = VuoSceneObject_bounds(layer.sceneObject);
//			VLog("bounds3d center = %s		size=%s",VuoPoint3d_getSummary(bounds.center),VuoPoint3d_getSummary(bounds.size));
			QCOMPARE(bounds.center.x + 10, expectedCenter3D.x + 10.);
			QCOMPARE(bounds.center.y + 10, expectedCenter3D.y + 10.);
			QCOMPARE(bounds.center.z + 10,                      10.);
			QCOMPARE(bounds.size.x   + 10, expectedSize3D.x   + 10.);
			QCOMPARE(bounds.size.y   + 10, expectedSize3D.y   + 10.);
			QCOMPARE(bounds.size.z   + 10,                      10.);
		}
	}
};

QTEST_APPLESS_MAIN(TestVuoLayer)

#include "TestVuoLayer.moc"
