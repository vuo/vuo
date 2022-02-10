/**
 * @file
 * TestVuoLayer implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoLayer.h"
#include "VuoList_VuoLayer.h"
#include "VuoSceneText.h"
#include "VuoRenderedLayers.h"

void VuoRenderedLayers_setRenderingDimensions(const VuoRenderedLayers renderedLayers, unsigned long int pixelsWide, unsigned long int pixelsHigh, float backingScaleFactor);
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

	// These tests should pass for any viewport size
	// (though for odd and/or smallish viewports (below around 400),
	// "two real size layers" may encounter rounding errors).
	// Choose something != VuoGraphicsWindowDefaultWidth
	// to ensure the bounds calculations aren't making assumptions
	// about the viewport size.
	const int viewportSize = VuoGraphicsWindowDefaultWidth * 1.25;

private slots:

	void addAnchorTest(VuoText name, VuoAnchor anchor, VuoPoint2d expectedCenter, VuoPoint2d expectedSize, VuoPoint2d expectedCenter3D, VuoPoint2d expectedSize3D, int expectedChildCount)
	{
		VuoLayer base = VuoLayer_makeColor(name, VuoColor_makeWithRGBA(1., 0., 1., 1.), VuoPoint2d_make(0, 0.), 0., 1., 1.);
		VuoLayer layer = VuoLayer_setAnchor(base, anchor, -1, -1, -1);
		QTest::newRow(name) << layer << expectedCenter << expectedSize << expectedCenter << expectedSize << expectedCenter3D << expectedSize3D << expectedChildCount;
	}

	void testBounds_data()
	{
		QTest::addColumn<VuoLayer>("layer");
		QTest::addColumn<VuoPoint2d>("expectedCenterKnownViewport");
		QTest::addColumn<VuoPoint2d>("expectedSizeKnownViewport");
		QTest::addColumn<VuoPoint2d>("expectedCenterUnknownViewport");
		QTest::addColumn<VuoPoint2d>("expectedSizeUnknownViewport");
		QTest::addColumn<VuoPoint2d>("expectedCenter3D");
		QTest::addColumn<VuoPoint2d>("expectedSize3D");
		QTest::addColumn<int>("expectedChildCount");

		QTest::newRow("empty scene")	<< VuoLayer_makeEmpty()
										<< VuoPoint2d_make(NAN,NAN) << VuoPoint2d_make(0,0)
										<< VuoPoint2d_make(NAN,NAN) << VuoPoint2d_make(0,0)
										<< VuoPoint2d_make(0,0) << VuoPoint2d_make(0,0) << 0;

		{
			VuoList_VuoLayer childLayers = VuoListCreate_VuoLayer();
			VuoListAppendValue_VuoLayer(childLayers, VuoLayer_make(VuoText_make("child1"), VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1), 1,1), VuoPoint2d_make(-1,0), 0, 1, VuoOrientation_Horizontal, 1));
			VuoListAppendValue_VuoLayer(childLayers, VuoLayer_make(VuoText_make("child2"), VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1), 1,1), VuoPoint2d_make( 0,1), 0, 1, VuoOrientation_Horizontal, 1));

			VuoLayer parent = VuoLayer_makeGroup(childLayers, VuoTransform2d_makeIdentity());
			QTest::newRow("two scaled layers")	<< parent
												<< VuoPoint2d_make(-.5,.5) << VuoPoint2d_make(2,2)
												<< VuoPoint2d_make(-.5,.5) << VuoPoint2d_make(2,2)
												<< VuoPoint2d_make(-.5,.5) << VuoPoint2d_make(2,2)
												<< 2;
		}

		{
			VuoList_VuoLayer childLayers = VuoListCreate_VuoLayer();
			VuoListAppendValue_VuoLayer(childLayers, VuoLayer_makeRealSize(VuoText_make("child1"), VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1), 2,2), VuoPoint2d_make(-1,0), 1, false));
			VuoListAppendValue_VuoLayer(childLayers, VuoLayer_makeRealSize(VuoText_make("child2"), VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1), 2,2), VuoPoint2d_make( 0,1), 1, true ));

			VuoLayer parent = VuoLayer_makeGroup(childLayers, VuoTransform2d_makeIdentity());
			QTest::newRow("two real size layers")	<< parent
													<< VuoPoint2d_make(-.5,.5) << VuoPoint2d_make(1 + 4./viewportSize, 1 + 4./viewportSize)
													<< VuoPoint2d_make(-.5,.5) << VuoPoint2d_make(1,1)
													<< VuoPoint2d_make(-.5,.5) << VuoPoint2d_make(1,1)
													<< 2;
		}

		VuoFont font = VuoFont_makeDefault();
		font.fontName = VuoText_make("Monaco");
		VuoFont_retain(font);

		// Check bounds of a non-scaled (deprecated) text layer (`vuo.layer.make.text`).
		// https://b33p.net/kosada/node/13853
		// https://b33p.net/kosada/node/16013
		{
			VuoSceneObject so = VuoSceneText_make(VuoText_make("text layer"), font, false, INFINITY, VuoAnchor_makeCentered());
			VuoSceneObject_setTransform(so, VuoTransform_makeEuler(VuoPoint3d_make(0,0,0), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)));

			QTest::newRow("non-scaled text layer") << (VuoLayer)so
												   << (VuoPoint2d){0,0} << (VuoPoint2d){213.6f/viewportSize, 48.f/viewportSize}
												   << (VuoPoint2d){0,0} << (VuoPoint2d){213.6f/1024, 48.f/1024}
												   << (VuoPoint2d){0,0} << (VuoPoint2d){ 0     , 0     }
												   << 0;
		}

		// Check bounds of a scaled text layer (`vuo.layer.make.text2`).
		// https://b33p.net/kosada/node/13853
		{
			VuoSceneObject so = VuoSceneText_make(VuoText_make("text layer"), font, true, INFINITY, VuoAnchor_makeCentered());
			VuoSceneObject_setTransform(so, VuoTransform_makeEuler(VuoPoint3d_make(0,0,0), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)));

			// We can calculate the size of scaled text layers, both with and without using VuoRenderedLayers.
			// (But in 3D scenes, text layers aren't valid.)
			QTest::newRow("scaled text layer") << (VuoLayer)so
											   << (VuoPoint2d){0,0} << (VuoPoint2d){0.2086, 0.0468807}
											   << (VuoPoint2d){0,0} << (VuoPoint2d){0.2086, 0.0468807}
											   << (VuoPoint2d){0,0} << (VuoPoint2d){0     , 0        }
											   << 0;
		}

		// Check bounds of an anchored, scaled text layer (`vuo.layer.make.text2`).
		// https://b33p.net/kosada/node/16049
		{
			VuoSceneObject so = VuoSceneText_make(VuoText_make("text layer"), font, true, INFINITY, VuoAnchor_make(VuoHorizontalAlignment_Left, VuoVerticalAlignment_Top));
			QTest::newRow("text layer anchor=topLeft") << (VuoLayer)so
													   << (VuoPoint2d){0.1043, -0.02344} << (VuoPoint2d){0.2086, 0.0468807}
													   << (VuoPoint2d){0.1043, -0.02344} << (VuoPoint2d){0.2086, 0.0468807}
													   << (VuoPoint2d){0,       0      } << (VuoPoint2d){0     , 0        }
													   << 0;
		}

		// Check bounds of a rotated, anchored, scaled text layer (`vuo.layer.make.text2`).
		// https://b33p.net/kosada/node/16049
		{
			VuoSceneObject so = VuoSceneText_make(VuoText_make("text layer"), font, true, INFINITY, VuoAnchor_make(VuoHorizontalAlignment_Left, VuoVerticalAlignment_Top));
			VuoSceneObject_setTransform(so, VuoTransform_makeEuler(VuoPoint3d_make(0,0,0), VuoPoint3d_make(0,0,15.*M_PI/180), VuoPoint3d_make(1,1,1)));
			QTest::newRow("text layer rot=15 anchor=topLeft") << (VuoLayer)so
													   << (VuoPoint2d){0.1068, 0.0043} << (VuoPoint2d){0.2136, 0.0993}
													   << (VuoPoint2d){0.1068, 0.0043} << (VuoPoint2d){0.2136, 0.0993}
													   << (VuoPoint2d){0,      0     } << (VuoPoint2d){0     , 0     }
													   << 0;
		}

		// A transformed text layer within a non-transformed group, with an empty layer.
		{
			VuoSceneObject so = VuoSceneText_make(VuoText_make("text layer"), font, true, INFINITY, VuoAnchor_makeCentered());
			VuoSceneObject_setTransform(so, VuoTransform_makeEuler(VuoPoint3d_make(0,0.1,0), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)));

			VuoLayer group = VuoLayer_makeGroup2((VuoLayer)so, VuoLayer_makeEmpty(), VuoTransform2d_makeIdentity());

			// We can calculate the size of scaled text layers, both with and without using VuoRenderedLayers.
			// (But in 3D scenes, text layers aren't valid.)
			QTest::newRow("transformed text layer in a group") << group
				<< (VuoPoint2d){0,0.1} << (VuoPoint2d){0.2086, 0.0468807}
				<< (VuoPoint2d){0,0.1} << (VuoPoint2d){0.2086, 0.0468807}
				<< (VuoPoint2d){0,0.1} << (VuoPoint2d){0     , 0        }
				<< 2;
		}

		// A non-transformed text layer within a transformed group, with an empty layer.
		{
			VuoSceneObject so = VuoSceneText_make(VuoText_make("text layer"), font, true, INFINITY, VuoAnchor_makeCentered());
			VuoSceneObject_setTransform(so, VuoTransform_makeEuler(VuoPoint3d_make(0,0,0), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)));

			VuoLayer group = VuoLayer_makeGroup2((VuoLayer)so, VuoLayer_makeEmpty(), VuoTransform2d_make((VuoPoint2d){0,.1}, 0, (VuoPoint2d){1,1}));

			// We can calculate the size of scaled text layers, both with and without using VuoRenderedLayers.
			// (But in 3D scenes, text layers aren't valid.)
			QTest::newRow("text layer in a transformed group") << group
				<< (VuoPoint2d){0,0.1} << (VuoPoint2d){0.2086, 0.0468807}
				<< (VuoPoint2d){0,0.1} << (VuoPoint2d){0.2086, 0.0468807}
				<< (VuoPoint2d){0,0.1} << (VuoPoint2d){0     , 0        }
				<< 2;
		}

		QTest::newRow("oval layer")	<< VuoLayer_makeOval(VuoText_make("child1"), VuoColor_makeWithRGBA(1,1,1,1), (VuoPoint2d){0,0}, 0, 1,1, 1)
									<< (VuoPoint2d){0,0} << (VuoPoint2d){1,1}
									<< (VuoPoint2d){0,0} << (VuoPoint2d){1,1}
									<< (VuoPoint2d){0,0} << (VuoPoint2d){1,1}
									<< 0;

		addAnchorTest(VuoText_make("Center Center Anchor"),
									VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Center),
									VuoPoint2d_make(0., 0.),	// center
									VuoPoint2d_make(1., 1.),	// size
									VuoPoint2d_make(0., 0.),	// expectedCenter3D,
									VuoPoint2d_make(1., 1.), 	// expectedSize3D
									0);

		addAnchorTest(VuoText_make("Bottom Left Anchor"),
									VuoAnchor_make(VuoHorizontalAlignment_Left, VuoVerticalAlignment_Bottom),
									VuoPoint2d_make(.5, .5),
									VuoPoint2d_make(1., 1.),
									VuoPoint2d_make(.5, .5),
									VuoPoint2d_make(1., 1.),
									1);

		addAnchorTest(VuoText_make("Center Left Anchor"),
									VuoAnchor_make(VuoHorizontalAlignment_Left, VuoVerticalAlignment_Center),
									VuoPoint2d_make(.5, 0.),
									VuoPoint2d_make(1., 1.),
									VuoPoint2d_make(.5, 0.),
									VuoPoint2d_make(1., 1.),
									1);

		addAnchorTest(VuoText_make("Top Left Anchor"),
									VuoAnchor_make(VuoHorizontalAlignment_Left, VuoVerticalAlignment_Top),
									VuoPoint2d_make(.5, -.5),
									VuoPoint2d_make(1., 1.),
									VuoPoint2d_make(.5, -.5),
									VuoPoint2d_make(1., 1.),
									1);

		addAnchorTest(VuoText_make("Top Center Anchor"),
									VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Top),
									VuoPoint2d_make(0., -.5),
									VuoPoint2d_make(1., 1.),
									VuoPoint2d_make(0., -.5),
									VuoPoint2d_make(1., 1.),
									1);

		addAnchorTest(VuoText_make("Top Right Anchor"),
									VuoAnchor_make(VuoHorizontalAlignment_Right, VuoVerticalAlignment_Top),
									VuoPoint2d_make(-.5, -.5),
									VuoPoint2d_make(1., 1.),
									VuoPoint2d_make(-.5, -.5),
									VuoPoint2d_make(1., 1.),
									1);

		addAnchorTest(VuoText_make("Center Right Anchor"),
									VuoAnchor_make(VuoHorizontalAlignment_Right, VuoVerticalAlignment_Center),
									VuoPoint2d_make(-.5, 0.),
									VuoPoint2d_make(1., 1.),
									VuoPoint2d_make(-.5, 0.),
									VuoPoint2d_make(1., 1.),
									1);

		addAnchorTest(VuoText_make("Bottom Right Anchor"),
									VuoAnchor_make(VuoHorizontalAlignment_Right, VuoVerticalAlignment_Bottom),
									VuoPoint2d_make(-.5, .5),
									VuoPoint2d_make(1., 1.),
									VuoPoint2d_make(-.5, .5),
									VuoPoint2d_make(1., 1.),
									1);

		addAnchorTest(VuoText_make("Bottom Center Anchor"),
									VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Bottom),
									VuoPoint2d_make(0., .5),
									VuoPoint2d_make(1., 1.),
									VuoPoint2d_make(0., .5),
									VuoPoint2d_make(1., 1.),
									1);
	}

	void testBounds()
	{
		QFETCH(VuoLayer, layer);
		QFETCH(VuoPoint2d, expectedCenterKnownViewport);
		QFETCH(VuoPoint2d, expectedSizeKnownViewport);
		QFETCH(VuoPoint2d, expectedCenterUnknownViewport);
		QFETCH(VuoPoint2d, expectedSizeUnknownViewport);
		QFETCH(VuoPoint2d, expectedCenter3D);
		QFETCH(VuoPoint2d, expectedSize3D);
		QFETCH(int, expectedChildCount);

		// Test VuoLayer_getBoundingRectangle() _without_ a known viewport size, as used by:
		// vuo.layer.arrange.grid
		// vuo.layer.bounds
		for (int backingScaleFactor = 1; backingScaleFactor <= 2; ++backingScaleFactor)
		{
			VuoRectangle bounds = VuoLayer_getBoundingRectangle(layer, -1, -1, backingScaleFactor);
//VLog("%gx%g @ %g,%g",bounds.size.x,bounds.size.y,bounds.center.x,bounds.center.y);
			VuoInteger childCount = VuoListGetCount_VuoSceneObject(VuoSceneObject_getChildObjects((VuoSceneObject)layer));
			VUO_COMPARE_NAN(bounds.center.x + 10, expectedCenterUnknownViewport.x + 10);
			VUO_COMPARE_NAN(bounds.center.y + 10, expectedCenterUnknownViewport.y + 10);
			int bsf = 1;
			if (QString(QTest::currentDataTag()) == "non-scaled text layer")
				bsf = backingScaleFactor;
			VUO_COMPARE_NAN(bounds.size.x   + 10, expectedSizeUnknownViewport.x * bsf + 10);
			VUO_COMPARE_NAN(bounds.size.y   + 10, expectedSizeUnknownViewport.y * bsf + 10);
			QCOMPARE(childCount, expectedChildCount);
		}

		// Test VuoLayer_getBoundingRectangle() _with_ a known viewport size, as used by:
		// vuo.layer.align.window
		for (int backingScaleFactor = 1; backingScaleFactor <= 2; ++backingScaleFactor)
		{
			VuoRectangle bounds = VuoLayer_getBoundingRectangle(layer, viewportSize, viewportSize, backingScaleFactor);
//VLog("%gx%g @ %g,%g",bounds.size.x,bounds.size.y,bounds.center.x,bounds.center.y);
			VuoInteger childCount = VuoListGetCount_VuoSceneObject(VuoSceneObject_getChildObjects((VuoSceneObject)layer));

			VuoPoint2d expectedCenterKnownViewport2 = expectedCenterKnownViewport;
			VuoPoint2d expectedSizeKnownViewport2 = expectedSizeKnownViewport;
			if (backingScaleFactor == 2
			 && QString(QTest::currentDataTag()) == "two real size layers")
			{
				// One of these layers has preservePhysicalSize=false;
				// adjust our expectations when rendering at retina resolution.
				double onePixel = 1. / viewportSize;
				expectedCenterKnownViewport2.x += onePixel;
				expectedCenterKnownViewport2.y += onePixel;
				expectedSizeKnownViewport2.x   += onePixel * 2;
				expectedSizeKnownViewport2.y   += onePixel * 2;
			}
			else if (backingScaleFactor == 2
				  && QString(QTest::currentDataTag()) == "non-scaled text layer")
			{
				expectedSizeKnownViewport2.x *= 2;
				expectedSizeKnownViewport2.y *= 2;
			}

			VUO_COMPARE_NAN(bounds.center.x + 10, expectedCenterKnownViewport2.x + 10);
			VUO_COMPARE_NAN(bounds.center.y + 10, expectedCenterKnownViewport2.y + 10);
			VUO_COMPARE_NAN(bounds.size.x   + 10, expectedSizeKnownViewport2.x   + 10);
			VUO_COMPARE_NAN(bounds.size.y   + 10, expectedSizeKnownViewport2.y   + 10);
			QCOMPARE(childCount, expectedChildCount);
		}

		// Also test VuoSceneObject_bounds(), which is used by vuo.layer.combine.center.
		// @todo Why doesn't vuo.layer.combine.center use VuoLayer_getBoundingRectangle()?
		{
			VuoBox bounds = VuoSceneObject_bounds((VuoSceneObject)layer);
			// VLog("bounds3d center = %s		size=%s",VuoPoint3d_getSummary(bounds.center),VuoPoint3d_getSummary(bounds.size));
			QCOMPARE(bounds.center.x + 10., expectedCenter3D.x + 10.);
			QCOMPARE(bounds.center.y + 10., expectedCenter3D.y + 10.);
			QCOMPARE(bounds.center.z + 10.,                      10.);
			QCOMPARE(bounds.size.x   + 10., expectedSize3D.x   + 10.);
			QCOMPARE(bounds.size.y   + 10., expectedSize3D.y   + 10.);
			QCOMPARE(bounds.size.z   + 10.,                      10.);
		}

		// Also test VuoRenderedLayers_getTransformedLayer(),
		// which is used by `vuo.layer.bounds.rendered2`.
		for (int backingScaleFactor = 1; backingScaleFactor <= 2; ++backingScaleFactor)
		{
			VuoRenderedLayers renderedLayers = VuoRenderedLayers_makeEmpty();
			VuoRenderedLayers_setRootSceneObject(renderedLayers, (VuoSceneObject)layer);

			// Simulate the part of VuoRenderedLayers_update that sets rendering dimensions since we don't have a window.
			VuoRenderedLayers_setRenderingDimensions(renderedLayers, viewportSize * backingScaleFactor, viewportSize * backingScaleFactor, backingScaleFactor);

			VuoSceneObject foundObject;
			VuoList_VuoSceneObject ancestorObjects = VuoListCreate_VuoSceneObject();
			VuoLocal(ancestorObjects);
			bool isLayerFound = VuoSceneObject_findById((VuoSceneObject)layer, VuoLayer_getId(layer), ancestorObjects, &foundObject);
			QVERIFY(isLayerFound);

			VuoPoint2d layerCorners[4];
			bzero(layerCorners, sizeof(layerCorners));
			VuoPoint2d newCenter = (VuoPoint2d){NAN,NAN};
			VuoRectangle bounds = (VuoRectangle){(VuoPoint2d){NAN,NAN},(VuoPoint2d){0,0}};
			if (VuoRenderedLayers_getTransformedLayer(renderedLayers, ancestorObjects, foundObject, &newCenter, layerCorners, true))
				bounds = VuoRenderedLayers_getBoundingBox(layerCorners);

			VuoPoint2d expectedCenterKnownViewport2 = expectedCenterKnownViewport;
			VuoPoint2d expectedSizeKnownViewport2 = expectedSizeKnownViewport;
			if (backingScaleFactor == 2
			 && QString(QTest::currentDataTag()) == "two real size layers")
			{
				// One of these layers has preservePhysicalSize=false;
				// adjust our expectations when rendering at retina resolution.
				double onePixel = 1. / viewportSize;
				expectedCenterKnownViewport2.x += onePixel / 2.;
				expectedCenterKnownViewport2.y += onePixel / 2.;
				expectedSizeKnownViewport2.x   -= onePixel;
				expectedSizeKnownViewport2.y   -= onePixel;
			}


			VUO_COMPARE_NAN(newCenter.x     + 10, expectedCenterKnownViewport2.x + 10);
			VUO_COMPARE_NAN(newCenter.y     + 10, expectedCenterKnownViewport2.y + 10);
			VUO_COMPARE_NAN(bounds.center.x + 10, expectedCenterKnownViewport2.x + 10);
			VUO_COMPARE_NAN(bounds.center.y + 10, expectedCenterKnownViewport2.y + 10);
			VUO_COMPARE_NAN(bounds.size.x   + 10, expectedSizeKnownViewport2.x   + 10);
			VUO_COMPARE_NAN(bounds.size.y   + 10, expectedSizeKnownViewport2.y   + 10);
		}
	}

	void testRenderedLayersTextSize_data()
	{
		QTest::addColumn<VuoLayer>("layer");
		QTest::addColumn<VuoPoint2d>("expectedSizeKnownViewport");
		QTest::addColumn<VuoPoint2d>("expectedSizeUnknownViewport");

		VuoFont font = VuoFont_makeDefault();
		font.fontName = VuoText_make("Monaco");
		VuoFont_retain(font);

		{
			VuoSceneObject so = VuoSceneText_make(VuoText_make("text layer"), font, false, INFINITY, VuoAnchor_makeCentered());

			// We can calculate the size of non-scaled text layers only when we know the viewport size.
			QTest::newRow("non-scaled text layer") << (VuoLayer)so
												   << (VuoPoint2d){213.6f/viewportSize, 48.f/viewportSize}
												   << (VuoPoint2d){0, 0};
		}

		{
			VuoSceneObject so = VuoSceneText_make(VuoText_make("text layer"), font, true, INFINITY, VuoAnchor_makeCentered());

			// We can calculate the size of scaled text layers whether the viewport size is known or unknown.
			QTest::newRow("scaled text layer") << (VuoLayer)so
											   << (VuoPoint2d){0.2086, 0.0468807}
											   << (VuoPoint2d){0.2086, 0.0468807};
		}
	}
	void testRenderedLayersTextSize()
	{
		QFETCH(VuoLayer, layer);
		QFETCH(VuoPoint2d, expectedSizeKnownViewport);
		QFETCH(VuoPoint2d, expectedSizeUnknownViewport);

		VuoRenderedLayers renderedLayers = VuoRenderedLayers_makeEmpty();
		VuoRenderedLayers_setRootSceneObject(renderedLayers, (VuoSceneObject)layer);

		VuoSceneObject foundObject;
		bool isLayerFound = VuoSceneObject_findWithType((VuoSceneObject)layer, VuoSceneObjectSubType_Text, nullptr, &foundObject);
		QVERIFY(isLayerFound);

		{
			VuoPoint2d actualSize = VuoRenderedLayers_getTextSize(renderedLayers, VuoSceneObject_getText(foundObject), VuoSceneObject_getTextFont(foundObject), VuoSceneObject_shouldTextScaleWithScene(foundObject), 1, 0, VuoSceneObject_getTextWrapWidth(foundObject), false);
			VUO_COMPARE_NAN(actualSize.x + 10, expectedSizeUnknownViewport.x + 10);
			VUO_COMPARE_NAN(actualSize.y + 10, expectedSizeUnknownViewport.y + 10);
		}

		{
			// Simulate the part of VuoRenderedLayers_update that sets rendering dimensions since we don't have a window.
			VuoRenderedLayers_setRenderingDimensions(renderedLayers, viewportSize, viewportSize, 1);

			VuoPoint2d actualSize = VuoRenderedLayers_getTextSize(renderedLayers, VuoSceneObject_getText(foundObject), VuoSceneObject_getTextFont(foundObject), VuoSceneObject_shouldTextScaleWithScene(foundObject), 1, 0, VuoSceneObject_getTextWrapWidth(foundObject), false);
			VUO_COMPARE_NAN(actualSize.x + 10, expectedSizeKnownViewport.x + 10);
			VUO_COMPARE_NAN(actualSize.y + 10, expectedSizeKnownViewport.y + 10);
		}
	}
};

QTEST_APPLESS_MAIN(TestVuoLayer)

#include "TestVuoLayer.moc"
