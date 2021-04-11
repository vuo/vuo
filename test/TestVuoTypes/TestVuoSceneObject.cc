/**
 * @file
 * TestVuoSceneObject implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoSceneObject.h"
#include "VuoSceneObjectGet.h"
#include "VuoSceneRenderer.h"
#include "VuoMeshParametric.h"
#include "VuoList_VuoSceneObject.h"
}

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoMultisample);
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
		VuoSceneObject rootSceneObject = VuoSceneObject_makeGroup(childObjects, VuoTransform_makeIdentity());
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
											   0, 1,
											   NULL);
		return VuoSceneObject_makeMesh(m, VuoShader_makeDefaultShader(), VuoTransform_makeIdentity());
	}

private slots:

	void testMakeAndSummaryAndSerialization_data()
	{
		QTest::addColumn<VuoSceneObject>("value");
		QTest::addColumn<QString>("summary");
		QTest::addColumn<QString>("json");

		QTest::newRow("emptystring")	<< VuoSceneObject_makeFromString("")
										<< "No object"
										<< QUOTE(null);

		{
			VuoSceneObject o = VuoSceneObject_makeMesh(
						VuoMesh_makeQuad(),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity());
			VuoSceneObject_setName(o, VuoText_make("quad"));
			QTest::newRow("quad")		<< o
										<< "<div>Object named \"quad\"</div>\n<div>4 vertices, 6 elements</div>\n<div>Identity transform (no change)</div>\n<div>ID 0</div>\n<div>0 child objects</div>\n<div>Shaders:<ul>\n<li>Default Shader (Checkerboard)</li></ul></div>"
										<< "";	// Don't test serialization since it includes object pointers.
		}

		{
			VuoImage image = VuoImage_make(42,0,640,480);
			VuoShader s = VuoShader_makeUnlitImageShader(image, 1);

			VuoSceneObject o = VuoSceneObject_makeMesh(
						VuoMesh_makeQuad(),
						s,
						VuoTransform_makeIdentity());

			QTest::newRow("quad image")	<< o
										<< "<div>4 vertices, 6 elements</div>\n<div>Identity transform (no change)</div>\n<div>ID 0</div>\n<div>0 child objects</div>\n<div>Shaders:<ul>\n<li>Image Shader (Unlit)</li></ul></div>"
										<< "";	// Don't test serialization since it includes object pointers.
		}

		{
			VuoSceneObject o = VuoSceneObject_makeMesh(
						VuoMesh_makeQuad(),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity());

			// Create parent of VuoSceneObject o.
			VuoList_VuoSceneObject o2ChildObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(o2ChildObjects, o);
			VuoSceneObject o2 = VuoSceneObject_makeGroup(o2ChildObjects, VuoTransform_makeIdentity());

			QTest::newRow("quad in group") << o2
										<< "<div>0 vertices, 0 elements</div>\n<div>Identity transform (no change)</div>\n<div>ID 0</div>\n<div>1 child object</div>\n<div>1 descendant</div>\n<div>Total, including descendants:</div>\n<div>4 vertices, 6 elements</div>\n<div>Shaders:<ul>\n<li>Default Shader (Checkerboard)</li></ul></div>"
										<< "";	// Don't test serialization since it includes object pointers.
		}

		{
			VuoSceneObject o = VuoSceneObject_makeMesh(
						VuoMesh_makeQuad(),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity());

			// Create parent of VuoSceneObject o.
			VuoList_VuoSceneObject o2ChildObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(o2ChildObjects, o);
			VuoSceneObject o2 = VuoSceneObject_makeGroup(o2ChildObjects, VuoTransform_makeIdentity());

			// Create parent of VuoSceneObject o2.
			VuoList_VuoSceneObject o3ChildObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(o3ChildObjects, o2);
			VuoSceneObject o3 = VuoSceneObject_makeGroup(o3ChildObjects, VuoTransform_makeIdentity());

			QTest::newRow("quad in group in group") << o3
											<< "<div>0 vertices, 0 elements</div>\n<div>Identity transform (no change)</div>\n<div>ID 0</div>\n<div>1 child object</div>\n<div>2 descendants</div>\n<div>Total, including descendants:</div>\n<div>4 vertices, 6 elements</div>\n<div>Shaders:<ul>\n<li>Default Shader (Checkerboard)</li></ul></div>"
											<< "";	// Don't test serialization since it includes object pointers.
		}

		{
			VuoTransform transform = VuoTransform_makeEuler(
						VuoPoint3d_make(42,43,44),
						VuoPoint3d_make(0,-.5,-1),
						VuoPoint3d_make(1,1,1)
					);
			VuoSceneObject o = VuoSceneObject_makePerspectiveCamera(
						VuoText_make("vuocam"),
						transform,
						42.0,
						1.0,
						20.0
					);
			QTest::newRow("perspective camera")		<< o
													<< "<div>Camera-perspective named \"vuocam\"</div>\n<div>At (42, 43, 44)</div>\n<div>Rotated (-0, 28.6479, 57.2958)</div>\n<div>42° field of view</div>\n<div>Shows objects between depth 1 and 20</div>"
													<< "{\"type\":\"camera-perspective\",\"cameraDistanceMin\":1.0,\"cameraDistanceMax\":20.0,\"cameraFieldOfView\":42.0,\"name\":\"vuocam\",\"transform\":{\"translation\":[42.0,43.0,44.0],\"eulerRotation\":[0.0,-0.5,-1.0],\"scale\":[1.0,1.0,1.0]}}";
		}

		{
			VuoTransform transform = VuoTransform_makeFromTarget(
						VuoPoint3d_make(42,43,44),
						VuoPoint3d_make(45,46,47),
						VuoPoint3d_make(0,1,0)
					);
			VuoSceneObject o = VuoSceneObject_makePerspectiveCamera(
						VuoText_make("vuocam"),
						transform,
						42.0,
						1.0,
						20.0
					);
			QTest::newRow("targeted perspective camera")	<< o
															<< "<div>Camera-perspective named \"vuocam\"</div>\n<div>At (42, 43, 44)</div>\n<div>Target (45, 46, 47)</div>\n<div>42° field of view</div>\n<div>Shows objects between depth 1 and 20</div>"
															<< QUOTE({"type":"camera-perspective","cameraDistanceMin":1.0,"cameraDistanceMax":20.0,"cameraFieldOfView":42.0,"name":"vuocam","transform":{"translation":[42.0,43.0,44.0],"target":[45.0,46.0,47.0],"upDirection":[0.0,1.0,0.0]}});
		}

		{
			VuoTransform transform = VuoTransform_makeFromTarget(
						VuoPoint3d_make(42,43,44),
						VuoPoint3d_make(45,46,47),
						VuoPoint3d_make(0,1,0)
					);
			VuoSceneObject o = VuoSceneObject_makeStereoCamera(
						VuoText_make("vuocam"),
						transform,
						42.0,
						1.0,
						20.0,
						1.0,
						0.5
					);
			QTest::newRow("targeted stereo camera")	<< o
													<< "<div>Camera-stereo named \"vuocam\"</div>\n<div>At (42, 43, 44)</div>\n<div>Target (45, 46, 47)</div>\n<div>42° field of view (stereoscopic)</div>\n<div>Shows objects between depth 1 and 20</div>"
													<< QUOTE({"type":"camera-stereo","cameraDistanceMin":1.0,"cameraDistanceMax":20.0,"cameraFieldOfView":42.0,"cameraConfocalDistance":1.0,"cameraIntraocularDistance":0.5,"name":"vuocam","transform":{"translation":[42.0,43.0,44.0],"target":[45.0,46.0,47.0],"upDirection":[0.0,1.0,0.0]}});
		}

		{
			VuoTransform transform = VuoTransform_makeEuler(
						VuoPoint3d_make(52,53,54),
						VuoPoint3d_make(0,-.5,-1),
						VuoPoint3d_make(1,1,1)
					);
			VuoSceneObject o = VuoSceneObject_makeOrthographicCamera(
						VuoText_make("vuocam ortho"),
						transform,
						2.0,
						3.0,
						22.0
					);
			QTest::newRow("orthographic camera")	<< o
													<< "<div>Camera-orthographic named \"vuocam ortho\"</div>\n<div>At (52, 53, 54)</div>\n<div>Rotated (-0, 28.6479, 57.2958)</div>\n<div>2 unit width</div>\n<div>Shows objects between depth 3 and 22</div>"
													<< "{\"type\":\"camera-orthographic\",\"cameraDistanceMin\":3.0,\"cameraDistanceMax\":22.0,\"cameraWidth\":2.0,\"name\":\"vuocam ortho\",\"transform\":{\"translation\":[52.0,53.0,54.0],\"eulerRotation\":[0.0,-0.5,-1.0],\"scale\":[1.0,1.0,1.0]}}";
		}

		{
			VuoSceneObject o = VuoSceneObject_makeAmbientLight(
						VuoColor_makeWithRGBA(0,0.5,1,1),
						0.5
					);
			QTest::newRow("ambient light")	<< o
											<< "<div>Light-ambient</div>\n<div>Color <span style='background-color:#007fff;'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span> 0.00, 0.50, 1.00, 1.00</div>\n<div>Brightness 0.5</div>"
											<< QUOTE({"type":"light-ambient","lightColor":{"r":0.0,"g":0.5,"b":1.0,"a":1.0},"lightBrightness":0.5,"name":"Ambient Light"});
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
											<< "<div>Light-point</div>\n<div>Color <span style='background-color:#007fff;'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span> 0.00, 0.50, 1.00, 1.00</div>\n<div>Brightness 0.5</div>\n<div>Position (1, 2, 3)</div>\n<div>Range 2.5 units (0.5 sharpness)</div>"
											<< QUOTE({"type":"light-point","lightColor":{"r":0.0,"g":0.5,"b":1.0,"a":1.0},"lightBrightness":0.5,"lightRange":2.5,"lightSharpness":0.5,"name":"Point Light","transform":{"translation":[1.0,2.0,3.0],"eulerRotation":[0.0,0.0,0.0],"scale":[1.0,1.0,1.0]}});
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
											<< "<div>Light-spot</div>\n<div>Color <span style='background-color:#007fff;'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span> 0.00, 0.50, 1.00, 1.00</div>\n<div>Brightness 0.5</div>\n<div>Position (1, 2, 3)</div>\n<div>Range 2.5 units (0.5 sharpness)</div>\n<div>Direction (1, 0, 0)</div>\n<div>Cone 45°</div>"
											<< QUOTE({"type":"light-spot","lightColor":{"r":0.0,"g":0.5,"b":1.0,"a":1.0},"lightBrightness":0.5,"lightRange":2.5,"lightSharpness":0.5,"lightCone":0.78539818525314331,"name":"Spot Light","transform":{"translation":[1.0,2.0,3.0],"eulerRotation":[0.0,0.0,0.0],"scale":[1.0,1.0,1.0]}});
		}

		{
			VuoTransform transform = VuoTransform_makeFromTarget(
						VuoPoint3d_make(52,53,54),
						VuoPoint3d_make(55,56,57),
						VuoPoint3d_make(0,1,0)
					);
			VuoSceneObject o = VuoSceneObject_makeOrthographicCamera(
						VuoText_make("vuocam ortho"),
						transform,
						2.0,
						3.0,
						22.0
					);
			QTest::newRow("targeted orthographic camera")	<< o
															<< "<div>Camera-orthographic named \"vuocam ortho\"</div>\n<div>At (52, 53, 54)</div>\n<div>Target (55, 56, 57)</div>\n<div>2 unit width</div>\n<div>Shows objects between depth 3 and 22</div>"
															<< QUOTE({"type":"camera-orthographic","cameraDistanceMin":3.0,"cameraDistanceMax":22.0,"cameraWidth":2.0,"name":"vuocam ortho","transform":{"translation":[52.0,53.0,54.0],"target":[55.0,56.0,57.0],"upDirection":[0.0,1.0,0.0]}});
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
			// Exclude the unique id from the serialization tests.
			json_object *actualJson = VuoSceneObject_getJson(value);
			if (actualJson)
				json_object_object_del(actualJson, "id");
			const char *actualJsonString = json_object_to_json_string_ext(actualJson, JSON_C_TO_STRING_PLAIN);
			QCOMPARE(QString::fromUtf8(actualJsonString), json);

			json_object *actualRoundtripJson = VuoSceneObject_getJson(VuoSceneObject_makeFromString(json.toUtf8().data()));
			if (actualRoundtripJson)
				json_object_object_del(actualRoundtripJson, "id");
			const char *actualRoundtripJsonString = json_object_to_json_string_ext(actualRoundtripJson, JSON_C_TO_STRING_PLAIN);
			QCOMPARE(QString::fromUtf8(actualRoundtripJsonString), json);
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
			VuoSceneObject o = VuoSceneObject_makeMesh(
						VuoMesh_makeQuad(),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity());
			QTest::newRow("quad at origin")	<< o
											<< VuoPoint3d_make(0,0,0) << VuoPoint3d_make(1,1,0)
											<< true;
		}

		{
			VuoMesh m = VuoMeshParametric_generate(0, "u", "v", "0", 4, 4, false, 1, 3, false, 3, 6, NULL);
			VuoSceneObject o = VuoSceneObject_makeMesh(
						m,
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity());
			QTest::newRow("off-center 2x3 square at origin")	<< o
																<< VuoPoint3d_make(2,4.5,0) << VuoPoint3d_make(2,3,0)
																<< true;
		}

		{
			VuoSceneObject o = VuoSceneObject_makeMesh(
						VuoMeshParametric_generate(0, "u", "v", "0", 2, 2, false, -1, 1, false, -1, 1, NULL),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeEuler(VuoPoint3d_make(1,2,3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)));
			QTest::newRow("centered 2x2 square, transformed off-center")	<< o
																			<< VuoPoint3d_make(1,2,3) << VuoPoint3d_make(2,2,0)
																			<< true;
		}

		{
			VuoSceneObject child0 = VuoSceneObject_makeMesh(
						VuoMeshParametric_generate(0, "u", "v", "0", 2, 2, false, -1, 1, false, -1, 1, NULL),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeEuler(VuoPoint3d_make(1,2,3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)));
			VuoSceneObject child1 = VuoSceneObject_makeMesh(
						VuoMeshParametric_generate(0, "u", "v", "0", 4, 4, false, 1, 3, false, 3, 6, NULL),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity());
			VuoList_VuoSceneObject childObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(childObjects, child0);
			VuoListAppendValue_VuoSceneObject(childObjects, child1);

			QTest::newRow("2 off-center") << VuoSceneObject_makeGroup(childObjects,
				VuoTransform_makeEuler(VuoPoint3d_make(-1,-2,-3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)))
													<< VuoPoint3d_make(0.5,1.5,-1.5) << VuoPoint3d_make(3,5,3)
													<< true;
		}

		{
			VuoSceneObject grandchildMesh = VuoSceneObject_makeMesh(
						VuoMeshParametric_generate(0, "u", "v", "0", 2, 2, false, -1, 1, false, -1, 1, NULL),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeEuler(VuoPoint3d_make(1,2,3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)));

			VuoList_VuoSceneObject grandchildObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(grandchildObjects, grandchildMesh);
			VuoSceneObject grandchildGroup = VuoSceneObject_makeGroup(grandchildObjects, VuoTransform_makeIdentity());

			VuoSceneObject childMesh = VuoSceneObject_makeMesh(
						VuoMeshParametric_generate(0, "u", "v", "0", 2, 2, false, -1, 1, false, -1, 1, NULL),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity());

			VuoList_VuoSceneObject childObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(childObjects, childMesh);
			VuoListAppendValue_VuoSceneObject(childObjects, grandchildGroup);
			VuoSceneObject childGroup = VuoSceneObject_makeGroup(childObjects, VuoTransform_makeEuler(VuoPoint3d_make(1,2,3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)));

			VuoSceneObject parentMesh = VuoSceneObject_makeMesh(
						VuoMeshParametric_generate(0, "u", "v", "0", 4, 4, false, 1, 3, false, 3, 6, NULL),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity());

			VuoList_VuoSceneObject groupObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(groupObjects, parentMesh);
			VuoListAppendValue_VuoSceneObject(groupObjects, childGroup);

			QTest::newRow("off-center with child and grandchild") << VuoSceneObject_makeGroup(groupObjects, VuoTransform_makeEuler(VuoPoint3d_make(-1,-2,-3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)))
																	<< VuoPoint3d_make(0.5,1.5,0) << VuoPoint3d_make(3,5,6)
																	<< true;
		}

		{
			VuoSceneObject grandchildMesh = VuoSceneObject_makeMesh(
						VuoMeshParametric_generate(0, "u", "v", "0", 2, 2, false, -1, 1, false, -1, 1, NULL),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeEuler(VuoPoint3d_make(1,2,3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)));

			VuoList_VuoSceneObject grandchildObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(grandchildObjects, grandchildMesh);
			VuoSceneObject grandchildGroup = VuoSceneObject_makeGroup(grandchildObjects, VuoTransform_makeIdentity());

			VuoSceneObject childMesh = VuoSceneObject_makeMesh(
						VuoMeshParametric_generate(0, "u", "v", "0", 2, 2, false, -1, 1, false, -1, 1, NULL),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity());

			VuoList_VuoSceneObject childObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(childObjects, childMesh);
			VuoListAppendValue_VuoSceneObject(childObjects, grandchildGroup);
			VuoSceneObject childGroup = VuoSceneObject_makeGroup(childObjects, VuoTransform_makeEuler(VuoPoint3d_make(1,2,3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)));

			VuoSceneObject parentMesh = VuoSceneObject_makeMesh(
						VuoMeshParametric_generate(0, "u", "v", "0", 4, 4, false, 1, 3, false, 3, 6, NULL),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity());

			VuoList_VuoSceneObject groupObjects = VuoListCreate_VuoSceneObject();
			VuoListAppendValue_VuoSceneObject(groupObjects, parentMesh);
			VuoListAppendValue_VuoSceneObject(groupObjects, childGroup);

			QTest::newRow("scaled, off-center with child and grandchild") << VuoSceneObject_makeGroup(groupObjects, VuoTransform_makeEuler(VuoPoint3d_make(-1,-2,-3), VuoPoint3d_make(0,0,0), VuoPoint3d_make(2,2,2)))
																			<< VuoPoint3d_make(2,5,3) << VuoPoint3d_make(6,10,12)
																			<< true;
		}

		{
			VuoSceneObject o = VuoSceneObject_makeMesh(
						VuoMeshParametric_generate(0, "u", "v", "0", 4, 4, false, 1, 3, false, 3, 6, NULL),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeEuler(VuoPoint3d_make(0,0,0), VuoPoint3d_make(0,0,M_PI/2), VuoPoint3d_make(1,1,1)));
			QTest::newRow("2x3 square, rotated")	<< o
													<< VuoPoint3d_make(-4.5,2,0) << VuoPoint3d_make(3,2,0)
													<< true;
		}

		{
			VuoSceneObject o = VuoSceneObject_makeMesh(
						VuoMesh_makeEquilateralTriangle(),
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity());
			double firstY = sin(M_PI/2)/sqrt(3);
			double secondY = sin(M_PI/2+2*M_PI/3)/sqrt(3);
			QTest::newRow("triangle at origin")	<< o
												<< VuoPoint3d_make(0, (firstY+secondY)/2, 0) << VuoPoint3d_make(1, firstY-secondY, 0)
												<< true;
		}

		{
			VuoList_VuoPoint3d positions = VuoListCreate_VuoPoint3d();
			VuoListAppendValue_VuoPoint3d(positions, (VuoPoint3d){-1,-1,-1});
			VuoListAppendValue_VuoPoint3d(positions, (VuoPoint3d){1,1,1});
			VuoMesh m = VuoMesh_make_VuoPoint3d(positions, NULL, VuoMesh_Points, .01);
			VuoSceneObject o = VuoSceneObject_makeMesh(
						m,
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity());
			QTest::newRow("points at origin")	<< o
												<< VuoPoint3d_make(0,0,0) << VuoPoint3d_make(2,2,2)
												<< true;
		}

		{
			VuoList_VuoPoint3d positions = VuoListCreate_VuoPoint3d();
			VuoListAppendValue_VuoPoint3d(positions, (VuoPoint3d){-1,-1,-1});
			VuoListAppendValue_VuoPoint3d(positions, (VuoPoint3d){1,1,1});
			VuoMesh m = VuoMesh_make_VuoPoint3d(positions, NULL, VuoMesh_IndividualLines, .01);
			VuoSceneObject o = VuoSceneObject_makeMesh(
						m,
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity());
			QTest::newRow("line at origin")	<< o
											<< VuoPoint3d_make(0,0,0) << VuoPoint3d_make(2,2,2)
											<< true;
		}

		{
			VuoList_VuoPoint3d positions = VuoListCreate_VuoPoint3d();
			VuoListAppendValue_VuoPoint3d(positions, (VuoPoint3d){-1,-1,-1});
			VuoListAppendValue_VuoPoint3d(positions, (VuoPoint3d){1,1,1});
			VuoMesh m = VuoMesh_make_VuoPoint3d(positions, NULL, VuoMesh_LineStrip, .01);
			VuoSceneObject o = VuoSceneObject_makeMesh(
						m,
						VuoShader_makeDefaultShader(),
						VuoTransform_makeIdentity());
			QTest::newRow("line strip at origin")	<< o
													<< VuoPoint3d_make(0,0,0) << VuoPoint3d_make(2,2,2)
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

		// Ensure the flattened scene's bounds match the expected bounds.
		{
			VuoSceneObject flattened = VuoSceneObject_flatten(scene);
			VuoBox flattenedBounds = VuoSceneObject_bounds(flattened);

			QCOMPARE(flattenedBounds.center.x + 10, expectedCenter.x + 10);
			QCOMPARE(flattenedBounds.center.y + 10, expectedCenter.y + 10);
			QCOMPARE(flattenedBounds.center.z + 10, expectedCenter.z + 10);
			QCOMPARE(flattenedBounds.size.x   + 10, expectedSize.x   + 10);
			QCOMPARE(flattenedBounds.size.y   + 10, expectedSize.y   + 10);
			QCOMPARE(flattenedBounds.size.z   + 10, expectedSize.z   + 10);
		}

		// Once the scene is centered, ensure the center is at the origin.
		{
			VuoSceneObject_center(scene);
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
			VuoSceneObject_normalize(scene);
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

	void testRenderEmptySceneToImagePerformance_data()
	{
		QTest::addColumn<VuoMultisample>("multisampling");

		QTest::newRow("1") << VuoMultisample_Off;
		QTest::newRow("2") << VuoMultisample_2;
		QTest::newRow("4") << VuoMultisample_4;
		QTest::newRow("8") << VuoMultisample_8;
	}
	void testRenderEmptySceneToImagePerformance()
	{
		QFETCH(VuoMultisample, multisampling);

		VuoSceneRenderer sr = VuoSceneRenderer_make(1);
		VuoRetain(sr);

		QBENCHMARK {
			// Copied from vuo.scene.render.image.

			VuoSceneObject rootSceneObject = VuoSceneObject_makeGroup(VuoListCreate_VuoSceneObject(), VuoTransform_makeIdentity());
			VuoSceneObject_retain(rootSceneObject);

			VuoSceneRenderer_setRootSceneObject(sr, rootSceneObject);
			VuoSceneRenderer_setCameraName(sr, VuoText_make(""), true);
			VuoSceneRenderer_regenerateProjectionMatrix(sr, 1920, 1080);
			VuoImage i;
			VuoSceneRenderer_renderToImage(sr, &i, VuoImageColorDepth_8, multisampling, NULL, false);

			VuoRetain(i);
			VuoRelease(i);

			VuoSceneObject_release(rootSceneObject);
		}

		VuoRelease(sr);
	}

	void testRenderSphereToImagePerformance_data()
	{
		QTest::addColumn<VuoMultisample>("multisampling");

		QTest::newRow("1") << VuoMultisample_Off;
		QTest::newRow("2") << VuoMultisample_2;
		QTest::newRow("4") << VuoMultisample_4;
		QTest::newRow("8") << VuoMultisample_8;
	}
	void testRenderSphereToImagePerformance()
	{
		QFETCH(VuoMultisample, multisampling);

		VuoSceneRenderer sr = VuoSceneRenderer_make(1);
		VuoRetain(sr);

		QBENCHMARK {
			VuoSceneObject rootSceneObject = makeSphere();
			VuoSceneObject_retain(rootSceneObject);

			// Copied from vuo.scene.render.image.
			VuoSceneRenderer_setRootSceneObject(sr, rootSceneObject);
			VuoSceneRenderer_setCameraName(sr, VuoText_make(""), true);
			VuoSceneRenderer_regenerateProjectionMatrix(sr, 1920, 1080);
			VuoImage i;
			VuoSceneRenderer_renderToImage(sr, &i, VuoImageColorDepth_8, multisampling, NULL, false);

			VuoRetain(i);
			VuoRelease(i);

			VuoSceneObject_release(rootSceneObject);
		}

		VuoRelease(sr);
	}

	/**
	 * Tests generating, uploading, and rendering a cube (similar to what "Render Scene to Image" does when it receives an event).
	 */
	void testRenderCubeToImagePerformance()
	{
		VuoSceneRenderer sr = VuoSceneRenderer_make(1);
		VuoRetain(sr);

		QBENCHMARK {
			VuoSceneObject rootSceneObject = makeCube();
			VuoSceneObject_retain(rootSceneObject);

			// Copied from vuo.scene.render.image.
			VuoSceneRenderer_setRootSceneObject(sr, rootSceneObject);
			VuoSceneRenderer_setCameraName(sr, VuoText_make(""), true);
			VuoSceneRenderer_regenerateProjectionMatrix(sr, 1920, 1080);
			VuoImage i;
			VuoSceneRenderer_renderToImage(sr, &i, VuoImageColorDepth_8, VuoMultisample_Off, NULL, false);

			VuoRetain(i);
			VuoRelease(i);

			VuoSceneObject_release(rootSceneObject);
		}

		VuoRelease(sr);
	}

	/**
	 * Pregenerates and uploads a cube, then just tests rendering the cube.
	 */
	void testRenderStaticCubeToImagePerformance()
	{
		VuoSceneRenderer sr = VuoSceneRenderer_make(1);
		VuoRetain(sr);

		VuoSceneObject rootSceneObject = makeCube();
		VuoSceneObject_retain(rootSceneObject);

		// Copied from vuo.scene.render.image.
		VuoSceneRenderer_setRootSceneObject(sr, rootSceneObject);
		VuoSceneRenderer_setCameraName(sr, VuoText_make(""), true);
		VuoSceneRenderer_regenerateProjectionMatrix(sr, 1920, 1080);

		QBENCHMARK {
			VuoImage i;
			VuoSceneRenderer_renderToImage(sr, &i, VuoImageColorDepth_8, VuoMultisample_Off, NULL, false);

			VuoRetain(i);
			VuoRelease(i);
		}

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
			VuoSceneObject_setTranslation(cube, VuoPoint3d_random(VuoPoint3d_make(-1,-1,-1), VuoPoint3d_make(1,1,1)));

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
		VuoSceneRenderer sr = VuoSceneRenderer_make(1);
		VuoRetain(sr);

		VuoSceneObject rootSceneObject = makeSphereInstances(2000);

		// Copied from vuo.scene.render.image.
		VuoSceneRenderer_setRootSceneObject(sr, rootSceneObject);
		VuoSceneRenderer_setCameraName(sr, VuoText_make(""), true);
		VuoSceneRenderer_regenerateProjectionMatrix(sr, 640, 480);

		// Render one to prime the caches.
		VuoImage i;
		VuoSceneRenderer_renderToImage(sr, &i, VuoImageColorDepth_8, VuoMultisample_Off, NULL, false);
		VuoRetain(i);
		VuoRelease(i);

		QBENCHMARK {
			VuoImage i;
			VuoSceneRenderer_renderToImage(sr, &i, VuoImageColorDepth_8, VuoMultisample_Off, NULL, false);
			VuoRetain(i);
			VuoRelease(i);
		}

		VuoSceneObject_release(rootSceneObject);
		VuoRelease(sr);
	}

	/**
	 * Tests performance of regenerating the projection matrix in a scene with a bunch of objects.
	 */
	void testRegenerateProjectionMatrixPerformance()
	{
		VuoSceneRenderer sr = VuoSceneRenderer_make(1);
		VuoRetain(sr);

		VuoSceneObject rootSceneObject = makeSphereInstances(20000);

		// Copied from vuo.scene.render.image.
		VuoSceneRenderer_setRootSceneObject(sr, rootSceneObject);
		VuoSceneRenderer_setCameraName(sr, VuoText_make(""), true);

		// Regenerate once to prime the caches.
		VuoSceneRenderer_regenerateProjectionMatrix(sr, 640, 480);

		QBENCHMARK {
			VuoSceneRenderer_regenerateProjectionMatrix(sr, 640, 480);
		}

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
			VuoSceneObject_normalize(so);
		}

		VuoSceneObject_release(so);
	}

	/**
	 * Tests finding transformed lights in a scene.
	 */
	void testFindLights()
	{
		VuoList_VuoSceneObject objects = VuoListCreate_VuoSceneObject();
		VuoLocal(objects);

		VuoListAppendValue_VuoSceneObject(objects, VuoSceneObject_makePointLight((VuoColor){1,0,0,1}, .4, (VuoPoint3d){1,2,3}, 1, 1));

		{
			VuoList_VuoSceneObject transformedObjects = VuoListCreate_VuoSceneObject();
			VuoLocal(transformedObjects);

			VuoListAppendValue_VuoSceneObject(transformedObjects, VuoSceneObject_makePointLight((VuoColor){0,1,0,1}, .4, (VuoPoint3d){1,2,3}, 1, 1));

			VuoTransform transform = VuoTransform_makeEuler(
				(VuoPoint3d){10,10,10},
				(VuoPoint3d){0,0,0},
				(VuoPoint3d){1,1,1});

			{
				VuoList_VuoSceneObject transformedObjects2 = VuoListCreate_VuoSceneObject();
				VuoLocal(transformedObjects2);

				VuoListAppendValue_VuoSceneObject(transformedObjects2, VuoSceneObject_makePointLight((VuoColor){0,0,1,1}, .4, (VuoPoint3d){1,2,3}, 1, 1));

				VuoListAppendValue_VuoSceneObject(transformedObjects, VuoSceneObject_makeGroup(transformedObjects2, transform));
			}

			VuoListAppendValue_VuoSceneObject(objects, VuoSceneObject_makeGroup(transformedObjects, transform));
		}

		VuoSceneObject so = VuoSceneObject_makeGroup(objects, VuoTransform_makeIdentity());
		VuoSceneObject_retain(so);

		VuoColor ambientColor;
		float ambientBrightness;
		VuoList_VuoSceneObject pointLights, spotLights;
		VuoSceneObject_findLights(so, &ambientColor, &ambientBrightness, &pointLights, &spotLights);

		QVERIFY(VuoColor_areEqual(ambientColor, (VuoColor){0,0,0,0}));
		QVERIFY(VuoReal_areEqual(ambientBrightness, 0));
		QCOMPARE(VuoListGetCount_VuoSceneObject(pointLights), 3UL);
		QVERIFY(VuoPoint3d_areEqual(VuoSceneObject_getTranslation(VuoListGetValue_VuoSceneObject(pointLights, 1)), (VuoPoint3d){1,2,3}));
		QVERIFY(VuoPoint3d_areEqual(VuoSceneObject_getTranslation(VuoListGetValue_VuoSceneObject(pointLights, 2)), (VuoPoint3d){11,12,13}));
		QVERIFY(VuoPoint3d_areEqual(VuoSceneObject_getTranslation(VuoListGetValue_VuoSceneObject(pointLights, 3)), (VuoPoint3d){21,22,23}));
		QCOMPARE(VuoListGetCount_VuoSceneObject(spotLights), 0UL);

		VuoRetain(pointLights);
		VuoRelease(pointLights);
		VuoRetain(spotLights);
		VuoRelease(spotLights);
		VuoSceneObject_release(so);
	}

	/**
	 * Tests performance of finding the lights in a scene with a bunch of objects (including instances).
	 */
	void testFindLightsPerformance()
	{
		VuoList_VuoSceneObject objects = VuoListCreate_VuoSceneObject();
		VuoRetain(objects);

		for (int i = 0; i < 20; ++i)
		{
			VuoListAppendValue_VuoSceneObject(objects, makeSphereInstances(200));

			if (i % 5 == 0)
			{
				VuoSceneObject light;
				if (i == 5)
					light = VuoSceneObject_makePointLight((VuoColor){1,0,0,1}, .4, (VuoPoint3d){0,0,0}, 1, 1);
				else if (i == 10)
					light = VuoSceneObject_makeSpotlight((VuoColor){0,1,0,1}, .4, VuoTransform_makeIdentity(), 1, 1, 1);
				else
					light = VuoSceneObject_makeAmbientLight((VuoColor){0,0,1,1}, .4);
				VuoListAppendValue_VuoSceneObject(objects, light);
			}
		}

		VuoSceneObject so = VuoSceneObject_makeGroup(objects, VuoTransform_makeIdentity());
		VuoSceneObject_retain(so);
		VuoRelease(objects);

		VuoColor ambientColor;
		float ambientBrightness;
		VuoList_VuoSceneObject pointLights, spotLights;
		QBENCHMARK {
			VuoSceneObject_findLights(so, &ambientColor, &ambientBrightness, &pointLights, &spotLights);
		}

		QVERIFY(VuoColor_areEqual(ambientColor, (VuoColor){0,0,1,1}));
		QVERIFY(VuoReal_areEqual(ambientBrightness, .8));
		QCOMPARE(VuoListGetCount_VuoSceneObject(pointLights), 1UL);
		QCOMPARE(VuoSceneObject_getType(VuoListGetValue_VuoSceneObject(pointLights, 1)), VuoSceneObjectSubType_PointLight);
		QCOMPARE(VuoListGetCount_VuoSceneObject(spotLights), 1UL);
		QCOMPARE(VuoSceneObject_getType(VuoListGetValue_VuoSceneObject(spotLights, 1)), VuoSceneObjectSubType_Spotlight);

		VuoRetain(pointLights);
		VuoRelease(pointLights);
		VuoRetain(spotLights);
		VuoRelease(spotLights);
		VuoSceneObject_release(so);
	}

	void testFetch_data()
	{
		QTest::addColumn<QString>("file");
		QTest::addColumn<unsigned long>("expectedDescendants");
		QTest::addColumn<unsigned long>("expectedVertices");
		QTest::addColumn<unsigned long>("expectedElements");

		// https://vuo.org/node/2064
		// https://b33p.net/kosada/node/13962
		QTest::newRow("scene with large mesh") << "vuoorg-2064.obj" << 3UL << 196608UL << 196608UL;
	}
	void testFetch()
	{
		QFETCH(QString, file);
		QFETCH(unsigned long, expectedDescendants);
		QFETCH(unsigned long, expectedVertices);
		QFETCH(unsigned long, expectedElements);

		VuoSceneObject scene;
		VuoText filename = VuoText_make(QString("resources/mesh/%1").arg(file).toUtf8().data());
		VuoLocal(filename);
		bool successfullyLoaded = VuoSceneObject_get(filename, &scene, false, false, false);
		QVERIFY(successfullyLoaded);

		unsigned long descendantCount = 0;
		unsigned long totalVertexCount = 0;
		unsigned long totalElementCount = 0;
		VuoSceneObject_getStatistics(scene, &descendantCount, &totalVertexCount, &totalElementCount);
		QCOMPARE(descendantCount, expectedDescendants);
		QCOMPARE(totalVertexCount, expectedVertices);
		QCOMPARE(totalElementCount, expectedElements);
	}
};

QTEST_APPLESS_MAIN(TestVuoSceneObject)

#include "TestVuoSceneObject.moc"
