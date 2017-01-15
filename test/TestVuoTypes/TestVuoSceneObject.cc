/**
 * @file
 * TestVuoSceneObject implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoSceneObject.h"
#include "VuoSceneRenderer.h"
#include "VuoMeshParametric.h"
#include "VuoList_VuoSceneObject.h"
}

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoPoint3d);
Q_DECLARE_METATYPE(VuoSceneObject);

/**
 * Tests the VuoSceneObject type.
 */
class TestVuoSceneObject : public QObject
{
	Q_OBJECT

	VuoSceneObject makeCube(void)
	{
		VuoList_VuoSceneObject childObjects = VuoListCreate_VuoSceneObject();
		VuoListAppendValue_VuoSceneObject(childObjects, VuoSceneObject_makeCube(
			VuoTransform_makeIdentity(),
			VuoShader_makeDefaultShader(),
			VuoShader_makeDefaultShader(),
			VuoShader_makeDefaultShader(),
			VuoShader_makeDefaultShader(),
			VuoShader_makeDefaultShader(),
			VuoShader_makeDefaultShader()));
		VuoSceneObject rootSceneObject = VuoSceneObject_make(NULL, NULL, VuoTransform_makeIdentity(), childObjects);
		return rootSceneObject;
	}

	VuoSceneObject makeSphere(void)
	{
		// Copied from vuo.mesh.make.sphere
		const char *xExp = "sin((u-.5)*360) * cos((v-.5)*180) / 2.";
		const char *yExp = "sin((v-.5)*180) / 2.";
		const char *zExp = "cos((u-.5)*360) * cos((v-.5)*180) / 2.";
		VuoMesh m = VuoMeshParametric_generate(0,
											   xExp, yExp, zExp,
											   16, 16,
											   false,		// close u
											   0, 1,
											   false,		// close v
											   0, 1);
		return VuoSceneObject_make(m, VuoShader_makeDefaultShader(), VuoTransform_makeIdentity(), NULL);
	}

private slots:

	void testMakeAndSummaryAndSerialization_data()
	{
		QTest::addColumn<VuoSceneObject>("value");
		QTest::addColumn<QString>("summary");
		QTest::addColumn<QString>("json");

		QTest::newRow("emptystring")	<< VuoSceneObject_makeFromString("")
										<< "0 vertices, 0 elements<br><br>identity transform (no change)<br><br>0 child objects"
										<< QUOTE({"type":"empty","transform":"identity"});

		{
			VuoSceneObject o = VuoSceneObject_make(
						VuoMesh_makeQuad(),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity(),
						VuoListCreate_VuoSceneObject()
					);
			QTest::newRow("quad")		<< o
										<< "4 vertices, 6 elements<br><br>identity transform (no change)<br><br>0 child objects<br><br>shaders:<ul><li>Default Shader (Checkerboard)</li></ul>"
										<< "";	// Don't test serialization since it includes object pointers.
		}

		{
			VuoGlContext glContext = VuoGlContext_use();

			VuoImage image = VuoImage_make(42,0,640,480);
			VuoShader s = VuoShader_makeUnlitImageShader(image, 1);

			VuoSceneObject o = VuoSceneObject_make(
						VuoMesh_makeQuad(),
						s,
						VuoTransform_makeIdentity(),
						VuoListCreate_VuoSceneObject()
					);

			QTest::newRow("quad image")	<< o
										<< "4 vertices, 6 elements<br><br>identity transform (no change)<br><br>0 child objects<br><br>shaders:<ul><li>Image Shader (Unlit)</li></ul>"
										<< "";	// Don't test serialization since it includes object pointers.

			VuoGlContext_disuse(glContext);
		}

		{
			VuoSceneObject o = VuoSceneObject_make(
						VuoMesh_makeQuad(),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity(),
						VuoListCreate_VuoSceneObject()
					);

			// Create parent of VuoSceneObject o.
			VuoList_VuoSceneObject o2ChildObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(o2ChildObjects, o);
			VuoSceneObject o2 = VuoSceneObject_make(
						VuoMesh_makeQuad(),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity(),
						o2ChildObjects
					);

			QTest::newRow("quad quad")	<< o2
										<< "4 vertices, 6 elements<br><br>identity transform (no change)<br><br>1 child object<br>1 descendant<br><br>total, including descendants:<br>8 vertices, 12 elements<br><br>shaders:<ul><li>Default Shader (Checkerboard)</li></ul>"
										<< "";	// Don't test serialization since it includes object pointers.
		}

		{
			VuoSceneObject o = VuoSceneObject_make(
						VuoMesh_makeQuad(),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity(),
						VuoListCreate_VuoSceneObject()
					);

			// Create parent of VuoSceneObject o.
			VuoList_VuoSceneObject o2ChildObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(o2ChildObjects, o);
			VuoSceneObject o2 = VuoSceneObject_make(
						VuoMesh_makeQuad(),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity(),
						o2ChildObjects
					);

			// Create parent of VuoSceneObject o2.
			VuoList_VuoSceneObject o3ChildObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(o3ChildObjects, o2);
			VuoSceneObject o3 = VuoSceneObject_make(
						VuoMesh_makeQuad(),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity(),
						o3ChildObjects
					);

			QTest::newRow("quad quad quad")	<< o3
											<< "4 vertices, 6 elements<br><br>identity transform (no change)<br><br>1 child object<br>2 descendants<br><br>total, including descendants:<br>12 vertices, 18 elements<br><br>shaders:<ul><li>Default Shader (Checkerboard)</li></ul>"
											<< "";	// Don't test serialization since it includes object pointers.
		}

		{
			VuoTransform transform = VuoTransform_makeEuler(
						VuoPoint3d_make(42,43,44),
						VuoPoint3d_make(0,-.5,-1),
						VuoPoint3d_make(1,1,1)
					);
			VuoSceneObject o = VuoSceneObject_makePerspectiveCamera(
						"vuocam",
						transform,
						42.0,
						1.0,
						20.0
					);
			QTest::newRow("perspective camera")		<< o
													<< "camera-perspective<br>at (42, 43, 44)<br>rotated (-0, 28.6479, 57.2958)<br>42° field of view<br>shows objects between depth 1 and 20"
													<< "{\"type\":\"camera-perspective\",\"cameraDistanceMin\":1,\"cameraDistanceMax\":20,\"cameraFieldOfView\":42,\"name\":\"vuocam\",\"transform\":{\"translation\":[42,43,44],\"eulerRotation\":[0,-0.5,-1],\"scale\":[1,1,1]}}";
		}

		{
			VuoTransform transform = VuoTransform_makeFromTarget(
						VuoPoint3d_make(42,43,44),
						VuoPoint3d_make(45,46,47),
						VuoPoint3d_make(0,1,0)
					);
			VuoSceneObject o = VuoSceneObject_makePerspectiveCamera(
						"vuocam",
						transform,
						42.0,
						1.0,
						20.0
					);
			QTest::newRow("targeted perspective camera")	<< o
															<< "camera-perspective<br>at (42, 43, 44)<br>target (45, 46, 47)<br>42° field of view<br>shows objects between depth 1 and 20"
															<< QUOTE({"type":"camera-perspective","cameraDistanceMin":1,"cameraDistanceMax":20,"cameraFieldOfView":42,"name":"vuocam","transform":{"translation":[42,43,44],"target":[45,46,47],"upDirection":[0,1,0]}});
		}

		{
			VuoTransform transform = VuoTransform_makeFromTarget(
						VuoPoint3d_make(42,43,44),
						VuoPoint3d_make(45,46,47),
						VuoPoint3d_make(0,1,0)
					);
			VuoSceneObject o = VuoSceneObject_makeStereoCamera(
						"vuocam",
						transform,
						42.0,
						1.0,
						20.0,
						1.0,
						0.5
					);
			QTest::newRow("targeted stereo camera")	<< o
													<< "camera-stereo<br>at (42, 43, 44)<br>target (45, 46, 47)<br>42° field of view (stereoscopic)<br>shows objects between depth 1 and 20"
													<< QUOTE({"type":"camera-stereo","cameraDistanceMin":1,"cameraDistanceMax":20,"cameraFieldOfView":42,"cameraConfocalDistance":1,"cameraIntraocularDistance":0.5,"name":"vuocam","transform":{"translation":[42,43,44],"target":[45,46,47],"upDirection":[0,1,0]}});
		}

		{
			VuoTransform transform = VuoTransform_makeEuler(
						VuoPoint3d_make(52,53,54),
						VuoPoint3d_make(0,-.5,-1),
						VuoPoint3d_make(1,1,1)
					);
			VuoSceneObject o = VuoSceneObject_makeOrthographicCamera(
						"vuocam ortho",
						transform,
						2.0,
						3.0,
						22.0
					);
			QTest::newRow("orthographic camera")	<< o
													<< "camera-orthographic<br>at (52, 53, 54)<br>rotated (-0, 28.6479, 57.2958)<br>2 unit width<br>shows objects between depth 3 and 22"
													<< "{\"type\":\"camera-orthographic\",\"cameraDistanceMin\":3,\"cameraDistanceMax\":22,\"cameraWidth\":2,\"name\":\"vuocam ortho\",\"transform\":{\"translation\":[52,53,54],\"eulerRotation\":[0,-0.5,-1],\"scale\":[1,1,1]}}";
		}

		{
			VuoSceneObject o = VuoSceneObject_makeAmbientLight(
						VuoColor_makeWithRGBA(0,0.5,1,1),
						0.5
					);
			QTest::newRow("ambient light")	<< o
											<< "light-ambient<br>color (0, 0.5, 1, 1)<br>brightness 0.5"
											<< QUOTE({"type":"light-ambient","lightColor":{"r":0,"g":0.5,"b":1,"a":1},"lightBrightness":0.5});
		}

		{
			VuoSceneObject o = VuoSceneObject_makePointLight(
						VuoColor_makeWithRGBA(0,0.5,1,1),
						0.5,
						VuoPoint3d_make(1,2,3),
						2.5,
						0.5
					);
			QTest::newRow("point light")	<< o
											<< "light-point<br>color (0, 0.5, 1, 1)<br>brightness 0.5<br>position (1, 2, 3)<br>range 2.5 units (0.5 sharpness)"
											<< QUOTE({"type":"light-point","lightColor":{"r":0,"g":0.5,"b":1,"a":1},"lightBrightness":0.5,"lightRange":2.5,"lightSharpness":0.5,"transform":{"translation":[1,2,3],"eulerRotation":[0,0,0],"scale":[1,1,1]}});
		}

		{
			VuoSceneObject o = VuoSceneObject_makeSpotlight(
						VuoColor_makeWithRGBA(0,0.5,1,1),
						0.5,
						VuoTransform_makeEuler(VuoPoint3d_make(1,2,3), VuoPoint3d_make(0, 0, 0), VuoPoint3d_make(1,1,1)),
						45 * M_PI/180.,
						2.5,
						0.5
					);
			QTest::newRow("spotlight")	<< o
											<< "light-spot<br>color (0, 0.5, 1, 1)<br>brightness 0.5<br>position (1, 2, 3)<br>range 2.5 units (0.5 sharpness)<br>direction (1, 0, 0)<br>cone 45°"
											<< QUOTE({"type":"light-spot","lightColor":{"r":0,"g":0.5,"b":1,"a":1},"lightBrightness":0.5,"lightRange":2.5,"lightSharpness":0.5,"lightCone":0.78539818525314331,"transform":{"translation":[1,2,3],"eulerRotation":[0,0,0],"scale":[1,1,1]}});
		}

		{
			VuoTransform transform = VuoTransform_makeFromTarget(
						VuoPoint3d_make(52,53,54),
						VuoPoint3d_make(55,56,57),
						VuoPoint3d_make(0,1,0)
					);
			VuoSceneObject o = VuoSceneObject_makeOrthographicCamera(
						"vuocam ortho",
						transform,
						2.0,
						3.0,
						22.0
					);
			QTest::newRow("targeted orthographic camera")	<< o
															<< "camera-orthographic<br>at (52, 53, 54)<br>target (55, 56, 57)<br>2 unit width<br>shows objects between depth 3 and 22"
															<< QUOTE({"type":"camera-orthographic","cameraDistanceMin":3,"cameraDistanceMax":22,"cameraWidth":2,"name":"vuocam ortho","transform":{"translation":[52,53,54],"target":[55,56,57],"upDirection":[0,1,0]}});
		}
	}
	void testMakeAndSummaryAndSerialization()
	{
		QFETCH(VuoSceneObject, value);
		QFETCH(QString, summary);
		QFETCH(QString, json);

		QCOMPARE(QString::fromUtf8(VuoSceneObject_getSummary(value)), summary);
		if (json.length())
		{
			QCOMPARE(QString::fromUtf8(VuoSceneObject_getString(value)), json);
			QCOMPARE(QString::fromUtf8(VuoSceneObject_getString(VuoSceneObject_makeFromString(json.toUtf8().data()))), json);
		}
	}

	void testBounds_data()
	{
		QTest::addColumn<VuoSceneObject>("scene");
		QTest::addColumn<VuoPoint3d>("expectedCenter");
		QTest::addColumn<VuoPoint3d>("expectedSize");
		QTest::addColumn<bool>("testNormalizedBounds");

		QTest::newRow("empty scene")	<< VuoSceneObject_makeFromString("")
										<< VuoPoint3d_make(0,0,0) << VuoPoint3d_make(0,0,0)
										<< false;

		{
			VuoSceneObject o = VuoSceneObject_make(
						VuoMesh_makeQuad(),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity(),
						VuoListCreate_VuoSceneObject()
					);
			QTest::newRow("quad at origin")	<< o
											<< VuoPoint3d_make(0,0,0) << VuoPoint3d_make(1,1,0)
											<< true;
		}

		{
			VuoMesh m = VuoMeshParametric_generate(0, "u", "v", "0", 4, 4, false, 1, 3, false, 3, 6);
			VuoSceneObject o = VuoSceneObject_make(
						m,
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity(),
						VuoListCreate_VuoSceneObject()
					);
			QTest::newRow("off-center 2x3 square at origin")	<< o
																<< VuoPoint3d_make(2,4.5,0) << VuoPoint3d_make(2,3,0)
																<< true;
		}

		{
			VuoSceneObject o = VuoSceneObject_make(
						VuoMeshParametric_generate(0, "u", "v", "0", 2, 2, false, -1, 1, false, -1, 1),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeEuler(VuoPoint3d_make(1,2,3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)),
						VuoListCreate_VuoSceneObject()
					);
			QTest::newRow("centered 2x2 square, transformed off-center")	<< o
																			<< VuoPoint3d_make(1,2,3) << VuoPoint3d_make(2,2,0)
																			<< true;
		}

		{
			VuoSceneObject child = VuoSceneObject_make(
						VuoMeshParametric_generate(0, "u", "v", "0", 2, 2, false, -1, 1, false, -1, 1),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeEuler(VuoPoint3d_make(1,2,3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)),
						VuoListCreate_VuoSceneObject()
					);

			VuoList_VuoSceneObject childObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(childObjects, child);

			VuoSceneObject parent = VuoSceneObject_make(
						VuoMeshParametric_generate(0, "u", "v", "0", 4, 4, false, 1, 3, false, 3, 6),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeEuler(VuoPoint3d_make(-1,-2,-3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)),
						childObjects
					);

			QTest::newRow("off-center with child")	<< parent
													<< VuoPoint3d_make(0.5,1.5,-1.5) << VuoPoint3d_make(3,5,3)
													<< true;
		}

		{
			VuoSceneObject grandchild = VuoSceneObject_make(
						VuoMeshParametric_generate(0, "u", "v", "0", 2, 2, false, -1, 1, false, -1, 1),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeEuler(VuoPoint3d_make(1,2,3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)),
						VuoListCreate_VuoSceneObject()
					);

			VuoList_VuoSceneObject grandchildObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(grandchildObjects, grandchild);

			VuoSceneObject child = VuoSceneObject_make(
						VuoMeshParametric_generate(0, "u", "v", "0", 2, 2, false, -1, 1, false, -1, 1),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeEuler(VuoPoint3d_make(1,2,3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)),
						grandchildObjects
					);

			VuoList_VuoSceneObject childObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(childObjects, child);

			VuoSceneObject parent = VuoSceneObject_make(
						VuoMeshParametric_generate(0, "u", "v", "0", 4, 4, false, 1, 3, false, 3, 6),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeEuler(VuoPoint3d_make(-1,-2,-3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)),
						childObjects
					);

			QTest::newRow("off-center with child and grandchild")	<< parent
																	<< VuoPoint3d_make(0.5,1.5,0) << VuoPoint3d_make(3,5,6)
																	<< true;
		}

		{
			VuoSceneObject grandchild = VuoSceneObject_make(
						VuoMeshParametric_generate(0, "u", "v", "0", 2, 2, false, -1, 1, false, -1, 1),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeEuler(VuoPoint3d_make(1,2,3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)),
						VuoListCreate_VuoSceneObject()
					);

			VuoList_VuoSceneObject grandchildObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(grandchildObjects, grandchild);

			VuoSceneObject child = VuoSceneObject_make(
						VuoMeshParametric_generate(0, "u", "v", "0", 2, 2, false, -1, 1, false, -1, 1),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeEuler(VuoPoint3d_make(1,2,3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)),
						grandchildObjects
					);

			VuoList_VuoSceneObject childObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(childObjects, child);

			VuoSceneObject parent = VuoSceneObject_make(
						VuoMeshParametric_generate(0, "u", "v", "0", 4, 4, false, 1, 3, false, 3, 6),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeEuler(VuoPoint3d_make(-1,-2,-3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(2,2,2)),
						childObjects
					);

			QTest::newRow("scaled, off-center with child and grandchild")	<< parent
																			<< VuoPoint3d_make(2,5,3) << VuoPoint3d_make(6,10,12)
																			<< true;
		}

		{
			VuoSceneObject o = VuoSceneObject_make(
						VuoMeshParametric_generate(0, "u", "v", "0", 4, 4, false, 1, 3, false, 3, 6),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeEuler(VuoPoint3d_make(0,0,0), VuoPoint3d_make(0,0,M_PI/2), VuoPoint3d_make(1,1,1)),
						VuoListCreate_VuoSceneObject()
					);
			QTest::newRow("2x3 square, rotated")	<< o
													<< VuoPoint3d_make(-4.5,2,0) << VuoPoint3d_make(3,2,0)
													<< true;
		}
	}
	void testBounds()
	{
		QFETCH(VuoSceneObject, scene);
		QFETCH(VuoPoint3d, expectedCenter);
		QFETCH(VuoPoint3d, expectedSize);
		QFETCH(bool, testNormalizedBounds);

		// Ensure the scene's initial bounds match the expected bounds.
		{
			VuoBox bounds = VuoSceneObject_bounds(scene);
//			VLog("initial bounds		center = %s		size=%s",VuoPoint3d_getSummary(bounds.center),VuoPoint3d_getSummary(bounds.size));

			QCOMPARE(bounds.center.x + 10, expectedCenter.x + 10);
			QCOMPARE(bounds.center.y + 10, expectedCenter.y + 10);
			QCOMPARE(bounds.center.z + 10, expectedCenter.z + 10);
			QCOMPARE(bounds.size.x   + 10, expectedSize.x   + 10);
			QCOMPARE(bounds.size.y   + 10, expectedSize.y   + 10);
			QCOMPARE(bounds.size.z   + 10, expectedSize.z   + 10);
		}

		// Once the scene is centered, ensure the center is at the origin.
		{
			VuoSceneObject_center(&scene);
			VuoBox bounds = VuoSceneObject_bounds(scene);
//			VLog("centered bounds		center = %s		size=%s",VuoPoint3d_getSummary(bounds.center),VuoPoint3d_getSummary(bounds.size));

			QCOMPARE(bounds.center.x + 10, 10.);
			QCOMPARE(bounds.center.y + 10, 10.);
			QCOMPARE(bounds.center.z + 10, 10.);
			QCOMPARE(bounds.size.x   + 10, expectedSize.x + 10);
			QCOMPARE(bounds.size.y   + 10, expectedSize.y + 10);
			QCOMPARE(bounds.size.z   + 10, expectedSize.z + 10);
		}

		// Once the scene is centered and normalized, ensure one of the dimensions is exactly 1, and the other 2 dimensions are <= 1.
		{
			VuoSceneObject_normalize(&scene);
			VuoBox bounds = VuoSceneObject_bounds(scene);
//			VLog("normalized bounds	center = %s		size=%s",VuoPoint3d_getSummary(bounds.center),VuoPoint3d_getSummary(bounds.size));

			QCOMPARE(bounds.center.x + 10, 10.);
			QCOMPARE(bounds.center.y + 10, 10.);
			QCOMPARE(bounds.center.z + 10, 10.);

			if (testNormalizedBounds)
				QVERIFY(fabs(bounds.size.x-1) < 0.00001
					 || fabs(bounds.size.y-1) < 0.00001
					 || fabs(bounds.size.z-1) < 0.00001);

			QVERIFY(bounds.size.x <= 1.00001);
			QVERIFY(bounds.size.y <= 1.00001);
			QVERIFY(bounds.size.z <= 1.00001);
		}
	}

	void testRenderEmptySceneToImagePerformance()
	{
		VuoGlContext glContext = VuoGlContext_use();
		VuoSceneRenderer sr = VuoSceneRenderer_make(glContext, 1);
		VuoRetain(sr);

		QBENCHMARK {
			// Copied from vuo.scene.render.image.

			VuoSceneObject rootSceneObject = VuoSceneObject_make(NULL, NULL, VuoTransform_makeIdentity(), VuoListCreate_VuoSceneObject());
			VuoSceneObject_retain(rootSceneObject);

			VuoSceneRenderer_setRootSceneObject(sr, rootSceneObject);
			VuoSceneRenderer_setCameraName(sr, "", true);
			VuoSceneRenderer_regenerateProjectionMatrix(sr, 1920, 1080);
			VuoImage i;
			VuoSceneRenderer_renderToImage(sr, &i, VuoImageColorDepth_8, VuoMultisample_Off, NULL);

			VuoRetain(i);
			VuoRelease(i);

			VuoSceneObject_release(rootSceneObject);
		}

		VuoRelease(sr);
		VuoGlContext_disuse(glContext);
	}

	void testRenderSphereToImagePerformance()
	{
		VuoGlContext glContext = VuoGlContext_use();
		VuoSceneRenderer sr = VuoSceneRenderer_make(glContext, 1);
		VuoRetain(sr);

		QBENCHMARK {
			VuoSceneObject rootSceneObject = makeSphere();
			VuoSceneObject_retain(rootSceneObject);

			// Copied from vuo.scene.render.image.
			VuoSceneRenderer_setRootSceneObject(sr, rootSceneObject);
			VuoSceneRenderer_setCameraName(sr, "", true);
			VuoSceneRenderer_regenerateProjectionMatrix(sr, 1920, 1080);
			VuoImage i;
			VuoSceneRenderer_renderToImage(sr, &i, VuoImageColorDepth_8, VuoMultisample_Off, NULL);

			VuoRetain(i);
			VuoRelease(i);

			VuoSceneObject_release(rootSceneObject);
		}

		VuoRelease(sr);
		VuoGlContext_disuse(glContext);
	}

	/**
	 * Tests generating, uploading, and rendering a cube (similar to what "Render Scene to Image" does when it receives an event).
	 */
	void testRenderCubeToImagePerformance()
	{
		VuoGlContext glContext = VuoGlContext_use();
		VuoSceneRenderer sr = VuoSceneRenderer_make(glContext, 1);
		VuoRetain(sr);

		QBENCHMARK {
			VuoSceneObject rootSceneObject = makeCube();
			VuoSceneObject_retain(rootSceneObject);

			// Copied from vuo.scene.render.image.
			VuoSceneRenderer_setRootSceneObject(sr, rootSceneObject);
			VuoSceneRenderer_setCameraName(sr, "", true);
			VuoSceneRenderer_regenerateProjectionMatrix(sr, 1920, 1080);
			VuoImage i;
			VuoSceneRenderer_renderToImage(sr, &i, VuoImageColorDepth_8, VuoMultisample_Off, NULL);

			VuoRetain(i);
			VuoRelease(i);

			VuoSceneObject_release(rootSceneObject);
		}

		VuoRelease(sr);
		VuoGlContext_disuse(glContext);
	}

	/**
	 * Pregenerates and uploads a cube, then just tests rendering the cube.
	 */
	void testRenderStaticCubeToImagePerformance()
	{
		VuoGlContext glContext = VuoGlContext_use();
		VuoSceneRenderer sr = VuoSceneRenderer_make(glContext, 1);
		VuoRetain(sr);

		VuoSceneObject rootSceneObject = makeCube();
		VuoSceneObject_retain(rootSceneObject);

		// Copied from vuo.scene.render.image.
		VuoSceneRenderer_setRootSceneObject(sr, rootSceneObject);
		VuoSceneRenderer_setCameraName(sr, "", true);
		VuoSceneRenderer_regenerateProjectionMatrix(sr, 1920, 1080);

		QBENCHMARK {
			VuoImage i;
			VuoSceneRenderer_renderToImage(sr, &i, VuoImageColorDepth_8, VuoMultisample_Off, NULL);

			VuoRetain(i);
			VuoRelease(i);
		}

		VuoGlContext_disuse(glContext);
		VuoSceneObject_release(rootSceneObject);
		VuoRelease(sr);
	}

	/**
	 * Makes a bunch of instances of a sphere.
	 *
	 * The returned sceneobject needs to be released by the caller.
	 */
	VuoSceneObject makeSphereInstances(int count)
	{
		VuoSceneObject cube = makeSphere();
		VuoSceneObject_retain(cube);
		VuoList_VuoSceneObject cubes = VuoListCreate_VuoSceneObject();
		VuoRetain(cubes);

		for (int i = 0; i < count; ++i)
		{
			// Give each cube a random position.
			cube.transform.translation = VuoPoint3d_random(VuoPoint3d_make(-1,-1,-1), VuoPoint3d_make(1,1,1));

			VuoListAppendValue_VuoSceneObject(cubes, cube);
		}

		VuoSceneObject so = VuoSceneObject_makeGroup(cubes, VuoTransform_makeIdentity());
		VuoSceneObject_retain(so);

		VuoRelease(cubes);
		VuoSceneObject_release(cube);

		return so;
	}

	/**
	 * Tests performance of rendering a bunch of instances of a sphere with the same shader.
	 */
	void testRenderSameShaderPerformance()
	{
		VuoGlContext glContext = VuoGlContext_use();
		VuoSceneRenderer sr = VuoSceneRenderer_make(glContext, 1);
		VuoRetain(sr);

		VuoSceneObject rootSceneObject = makeSphereInstances(2000);

		// Copied from vuo.scene.render.image.
		VuoSceneRenderer_setRootSceneObject(sr, rootSceneObject);
		VuoSceneRenderer_setCameraName(sr, "", true);
		VuoSceneRenderer_regenerateProjectionMatrix(sr, 640, 480);

		// Render one to prime the caches.
		VuoImage i;
		VuoSceneRenderer_renderToImage(sr, &i, VuoImageColorDepth_8, VuoMultisample_Off, NULL);
		VuoRetain(i);
		VuoRelease(i);

		QBENCHMARK {
			VuoImage i;
			VuoSceneRenderer_renderToImage(sr, &i, VuoImageColorDepth_8, VuoMultisample_Off, NULL);
			VuoRetain(i);
			VuoRelease(i);
		}

		VuoGlContext_disuse(glContext);
		VuoSceneObject_release(rootSceneObject);
		VuoRelease(sr);
	}

	/**
	 * Tests performance of regenerating the projection matrix in a scene with a bunch of objects.
	 */
	void testRegenerateProjectionMatrixPerformance()
	{
		VuoGlContext glContext = VuoGlContext_use();
		VuoSceneRenderer sr = VuoSceneRenderer_make(glContext, 1);
		VuoRetain(sr);

		VuoSceneObject rootSceneObject = makeSphereInstances(20000);

		// Copied from vuo.scene.render.image.
		VuoSceneRenderer_setRootSceneObject(sr, rootSceneObject);
		VuoSceneRenderer_setCameraName(sr, "", true);

		// Regenerate once to prime the caches.
		VuoSceneRenderer_regenerateProjectionMatrix(sr, 640, 480);

		QBENCHMARK {
			VuoSceneRenderer_regenerateProjectionMatrix(sr, 640, 480);
		}

		VuoGlContext_disuse(glContext);
		VuoSceneObject_release(rootSceneObject);
		VuoRelease(sr);
	}

	/**
	 * Tests performance of normalizing a scene with a bunch of objects (including instances).
	 */
	void testNormalizePerformance()
	{
		VuoList_VuoSceneObject objects = VuoListCreate_VuoSceneObject();
		VuoRetain(objects);

		for (int i = 0; i < 20; ++i)
			VuoListAppendValue_VuoSceneObject(objects, makeSphereInstances(200));

		VuoSceneObject so = VuoSceneObject_makeGroup(objects, VuoTransform_makeIdentity());
		VuoSceneObject_retain(so);
		VuoRelease(objects);

		QBENCHMARK {
			VuoSceneObject_normalize(&so);
		}

		VuoSceneObject_release(so);
	}
};

QTEST_APPLESS_MAIN(TestVuoSceneObject)

#include "TestVuoSceneObject.moc"
