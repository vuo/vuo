/**
 * @file
 * vuo.scene.make.camera.perspective node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "../vuo.mouse/VuoMouse.h"

VuoModuleMetadata({
					 "title" : "Make Draggable Camera",
					 "keywords" : [ "frustum", "projection", "opengl", "scenegraph", "graphics",
						 "trackball", "ViewCube", "gizmo",
						 "navigate", "view", "browse",
						 "pan", "dolly", "roll", "yaw", "pitch" ],
					 "version" : "1.0.1",
					 "dependencies" : [ "VuoMouse" ],
					 "node": {
						 "exampleCompositions" : [ "DisplayCube.vuo", "DisplayScene.vuo" ]
					 }
				 });

#define DEG2RAD 0.0174532925f								///< Convert degrees to radians.
#define RAD2DEG 57.295779513f								///< Convert radians to degrees.

const VuoPoint3d VuoPoint3d_zero 	= (VuoPoint3d) {0,0,0};	///< Shorthand for VuoPoint3d_make(0, 0, 0)
const VuoPoint3d VuoPoint3d_one 	= (VuoPoint3d) {1,1,1};	///< Shorthand for VuoPoint3d_make(1, 1, 1)
const VuoPoint3d VuoPoint3d_right 	= (VuoPoint3d) {1,0,0};	///< Shorthand for VuoPoint3d_make(1, 0, 0)
const VuoPoint3d VuoPoint3d_up  	= (VuoPoint3d) {0,1,0};	///< Shorthand for VuoPoint3d_make(0, 1, 0)
const VuoPoint3d VuoPoint3d_forward	= (VuoPoint3d) {0,0,1};	///< Shorthand for VuoPoint3d_make(0, 0, 1)

const float ScrollModifier = 0.8;							///< When scrolling, apply this modifier to the movement distance.

struct nodeInstanceData
{
	// The camera to apply the trackball transform to.
	VuoSceneObject camera;

	// Camera farClip - nearClip.  Used to clamp zoom values such that the target is always in sight.
	float cameraRange;

	// A reference to the window being renderered to.  Used when calculating how much rotation / pan / zoom should be applied by
	// mouse deltas.
	VuoWindowReference window;

	// Window height in VuoCoordinates.  If no window is referenced, this is set to (screen aspect ratio * 2).
	float windowHeight;

	// Used when no window reference is available to normalize screen points.
	VuoPoint2d screenPixelDimensions;

	// How far the camera currently is from the target.
	float distance;

	// The point that this camera is looking at.
	VuoPoint3d target;

	// The last received mouse position.
	VuoPoint2d mousePosition;

	// Mouse listeners.
	VuoMouse *leftMouseDraggedListener;
	VuoMouse *middleMouseDraggedListener;
	VuoMouse *rightMouseDraggedListener;
	VuoMouse *mousePressedListener;
	VuoMouse *mouseScrolledListener;

	// Camera trasnform has been updated trigger.
	void (*updatedCamera)(VuoSceneObject camera);
};

/**
 *	Update nodeInstanceData's cached window height (in VuoCoordinates) and size (in pixels).
 */
void updateWindowDimensions(struct nodeInstanceData* instance)
{
	float width = 2.;

	int64_t pixelsWide, pixelsHigh;
	float backingScaleFactor = 1;

	if (!instance->window)
		VuoMouse_GetScreenDimensions(&pixelsWide, &pixelsHigh);
	else
		VuoWindowReference_getContentSize(instance->window, &pixelsWide, &pixelsHigh, &backingScaleFactor);

	float aspectRatio = (VuoReal)pixelsWide / pixelsHigh;

	instance->windowHeight = width / aspectRatio;
	instance->screenPixelDimensions = VuoPoint2d_make( (VuoReal)pixelsWide, (VuoReal)pixelsHigh );
}

/**
 *	Initialize the nodeInstanceData with default transform.
 */
struct nodeInstanceData * nodeInstanceInit(
		VuoInputData(VuoText, {"default":"camera"}) name,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoInputData(VuoReal) fieldOfView,
		VuoInputData(VuoReal, {"default":0.1}) distanceMin,	// Near clip.
		VuoInputData(VuoReal, {"default":10.0}) distanceMax	// Far clip.
	)
{

	struct nodeInstanceData* instance = (struct nodeInstanceData*)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	VuoTransform transform = VuoTransform_makeEuler(	VuoPoint3d_make(0., 0., 1.),
														VuoPoint3d_make(0., 0., 0.),
														VuoPoint3d_make(1., 1., 1.) );

	instance->cameraRange = distanceMax-distanceMin;

	instance->distance = 1.;
	instance->target = VuoPoint3d_zero;

	instance->camera = VuoSceneObject_makePerspectiveCamera(name, transform, fieldOfView, distanceMin, distanceMax);

	instance->leftMouseDraggedListener = VuoMouse_make();
	instance->middleMouseDraggedListener = VuoMouse_make();
	instance->rightMouseDraggedListener = VuoMouse_make();
	instance->mousePressedListener = VuoMouse_make();
	instance->mouseScrolledListener = VuoMouse_make();

	VuoRetain(instance->leftMouseDraggedListener);
	VuoRetain(instance->middleMouseDraggedListener);
	VuoRetain(instance->rightMouseDraggedListener);
	VuoRetain(instance->mousePressedListener);
	VuoRetain(instance->mouseScrolledListener);

	instance->window = window;

	updateWindowDimensions(instance);

	return instance;
};

/**
 *	Sets the camera transform back to default values and fires an event.
 */
void resetCamera(struct nodeInstanceData* instance)
{
	instance->camera.transform = VuoTransform_makeEuler(VuoPoint3d_make(0., 0., 1.),
														VuoPoint3d_zero,
														VuoPoint3d_one);
	instance->distance = 1.;
	instance->target = VuoPoint3d_zero;

	instance->updatedCamera(instance->camera);
}

/**
 *	Converts a screen space point to a VuoCoordinate.
 */
static VuoPoint2d convertScreenToVuoPoint(struct nodeInstanceData *instance, VuoPoint2d point)
{
	VuoPoint2d vuoPoint = VuoPoint2d_divide(point, instance->screenPixelDimensions);
	vuoPoint.x *= 2.;
	vuoPoint.y *= -instance->windowHeight;

	// to make it a real vuo coordinate you'd still want to do -1, but since all we care about
	// here is delta there's no need for the extra step.

	return vuoPoint;
}

/**
 *	Rotates a direction by rotation.
 */
VuoPoint3d transformDirection(VuoPoint3d rotation, VuoPoint3d direction)
{
	// Make a new transform with only the rotational component.
	VuoTransform t = VuoTransform_makeEuler(VuoPoint3d_zero, rotation, VuoPoint3d_one);

	float m[16];
	VuoTransform_getMatrix(t, m);

	return VuoTransform_transformPoint(m, direction);
}

/**
 *	Given a target, a rotation (euler in rads), and distance, return the camera's position.
 */
VuoPoint3d calculateCameraPosition(VuoPoint3d target, VuoPoint3d rotation, float distance)
{
	VuoPoint3d dir = transformDirection(rotation, VuoPoint3d_forward);

	return VuoPoint3d_add(VuoPoint3d_multiply(dir, distance), target);
}

/**
 *	Rotate the camera around target.
 */
void onLeftMouseDragged(struct nodeInstanceData *instance, VuoPoint2d point)
{
	VuoPoint2d vuoPoint = instance->window ? point : convertScreenToVuoPoint(instance, point);

	VuoPoint2d delta = VuoPoint2d_subtract(vuoPoint, instance->mousePosition);
	instance->mousePosition = vuoPoint;

	VuoPoint3d rotation = instance->camera.transform.rotationSource.euler;

	// Delta of 2 would be 360 deg rotation

	rotation.x += (delta.y * (360/instance->windowHeight)) * DEG2RAD;	// To invert Y movement, change to subtraction
	rotation.y -= (delta.x * 180) * DEG2RAD;

	// To restrict the camera to a 180 degree range pitch, uncomment (makes it harder to get "lost").
	// rotation.x = VuoReal_clamp(rotation.x, -90 * DEG2RAD, 90 * DEG2RAD);

	VuoPoint3d translation = calculateCameraPosition( instance->target, rotation, instance->distance);

	instance->camera.transform = VuoTransform_makeEuler(	translation,
															rotation,
															VuoPoint3d_one );

	instance->updatedCamera(instance->camera);
}

/**
 *	Pan the camera target up or down in model space.
 */
void onMiddleMouseDragged(struct nodeInstanceData* instance, VuoPoint2d point)
{
	VuoPoint2d vuoPoint = instance->window ? point : convertScreenToVuoPoint(instance, point);

	VuoPoint2d delta = VuoPoint2d_subtract(vuoPoint, instance->mousePosition);

	instance->mousePosition = vuoPoint;

	VuoPoint3d rotation = instance->camera.transform.rotationSource.euler;

	VuoPoint3d up = transformDirection(rotation, VuoPoint3d_up);		// some 3d apps lock y panning to the world axis, maybe make that an option?
	VuoPoint3d right = transformDirection(rotation, VuoPoint3d_right);

	up = VuoPoint3d_multiply(up, -delta.y);
	right = VuoPoint3d_multiply(right, -delta.x);

	instance->target = VuoPoint3d_add(instance->target, VuoPoint3d_add(up, right));

	instance->camera.transform.translation = calculateCameraPosition(instance->target, rotation, instance->distance);

	instance->updatedCamera(instance->camera);
}

/**
 *	Roll the camera while still looking at target.
 */
void onRightMouseDragged(struct nodeInstanceData *instance, VuoPoint2d point)
{
	VuoPoint2d vuoPoint = instance->window ? point : convertScreenToVuoPoint(instance, point);

	VuoPoint2d delta = VuoPoint2d_subtract(vuoPoint, instance->mousePosition);
	instance->mousePosition = vuoPoint;

	VuoPoint3d rotation = instance->camera.transform.rotationSource.euler;

	rotation.z += (delta.y * (360/instance->windowHeight)) * DEG2RAD;

	VuoPoint3d translation = calculateCameraPosition(instance->target, rotation, instance->distance);

	instance->camera.transform = VuoTransform_makeEuler(	translation,
															rotation,
															VuoPoint3d_one );

	instance->updatedCamera(instance->camera);
}

/**
 *	Reset the starting point for calculating drag delta
 */
void onMousePressed(struct nodeInstanceData *instance, VuoPoint2d point)
{
	VuoPoint2d vuoPoint = instance->window ? point : convertScreenToVuoPoint(instance, point);

	instance->mousePosition = vuoPoint;
}

/**
 *	Zoom in or out.
 */
void onMouseScrolled(struct nodeInstanceData *instance, VuoPoint2d delta)
{
	instance->distance -= delta.y * (instance->distance/(instance->cameraRange)) * ScrollModifier;

	// Clamp camera distance to near and far clip values
	if(instance->distance < instance->camera.cameraDistanceMin)
		instance->distance = instance->camera.cameraDistanceMin;

	if(instance->distance > instance->camera.cameraDistanceMax)
		instance->distance = instance->camera.cameraDistanceMax;

	instance->camera.transform.translation = calculateCameraPosition(	instance->target,
																		instance->camera.transform.rotationSource.euler,
																		instance->distance);

	instance->updatedCamera(instance->camera);
}

/**
 *	Begin listening with modifier key and window.
 */
void startMouseListeners(struct nodeInstanceData* instance, VuoModifierKey modifier)
{
	// Only fire drag events within the window content area, so dragging the titlebar doesn't cause the scene to rotate.
	// (Especially bad when you drag the window to the top of the screen, and the menu bar stops the window from moving up further, yet you can still rotate the scene.)
	// https://b33p.net/kosada/node/8934#comment-48225
	bool fireRegardlessOfPosition = false;

	VuoMouse_startListeningForDragsWithCallback( instance->leftMouseDraggedListener,
											^(VuoPoint2d point) { onLeftMouseDragged(instance, point); },
											VuoMouseButton_Left,
											instance->window,
											modifier,
											fireRegardlessOfPosition);

	VuoMouse_startListeningForDragsWithCallback( instance->middleMouseDraggedListener,
											^(VuoPoint2d point) { onMiddleMouseDragged(instance, point); },
											VuoMouseButton_Middle,
											instance->window,
											modifier,
											fireRegardlessOfPosition);

	VuoMouse_startListeningForDragsWithCallback( instance->rightMouseDraggedListener,
											^(VuoPoint2d point) { onRightMouseDragged(instance, point); },
											VuoMouseButton_Right,
											instance->window,
											modifier,
											fireRegardlessOfPosition);

	VuoMouse_startListeningForPressesWithCallback( instance->mousePressedListener,
											^(VuoPoint2d point) {onMousePressed(instance, point); },
											VuoMouseButton_Any,
											instance->window,
											modifier
											);

	VuoMouse_startListeningForScrollsWithCallback( instance->mouseScrolledListener,
											^(VuoPoint2d delta) { onMouseScrolled(instance, delta); },
											instance->window,
											modifier
											);
}

void stopMouseListeners(struct nodeInstanceData* instance)
{
	VuoMouse_stopListening(instance->leftMouseDraggedListener);
	VuoMouse_stopListening(instance->middleMouseDraggedListener);
	VuoMouse_stopListening(instance->rightMouseDraggedListener);
	VuoMouse_stopListening(instance->mousePressedListener);
	VuoMouse_stopListening(instance->mouseScrolledListener);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoText, {"default":"camera"}) name,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoInputData(VuoWindowReference) window,
		VuoInputEvent({"data":"window"}) windowEvent,
		VuoInputData(VuoReal, {"name":"Field of View", "default":90.0, "suggestedMin":0.01, "suggestedMax":179.9, "suggestedStep":1.0}) fieldOfView,
		VuoInputData(VuoReal, {"default":0.1}) distanceMin,		// Near clip.
		VuoInputData(VuoReal, {"default":10.0}) distanceMax,	// Far clip.
		VuoInputEvent({"eventBlocking":"none"}) reset,
		VuoOutputTrigger(updatedCamera, VuoSceneObject)
)
{
	(*instance)->window = window;

	updateWindowDimensions(*instance);

	stopMouseListeners(*instance);
	startMouseListeners(*instance, modifierKey);

	if(reset)
		resetCamera(*instance);

	VuoSceneObject* camera = &(*instance)->camera;

	camera->cameraFieldOfView = fieldOfView;
	camera->cameraDistanceMin = distanceMin;
	camera->cameraDistanceMax = distanceMax;

	(*instance)->cameraRange = distanceMax-distanceMin;

	updatedCamera(*camera);
}

void nodeInstanceTriggerStart(
	VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoInputData(VuoWindowReference) window,
		VuoOutputTrigger(updatedCamera, VuoSceneObject)
	)
{
	(*instance)->window = window;
	(*instance)->updatedCamera = updatedCamera;

	startMouseListeners(*instance, modifierKey);

	// Fire one on start just so the Camera starts rendering
	updatedCamera((*instance)->camera);
}

void nodeInstanceTriggerStop(VuoInstanceData(struct nodeInstanceData *) instance)
{
		stopMouseListeners(*instance);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->leftMouseDraggedListener);
	VuoRelease((*instance)->middleMouseDraggedListener);
	VuoRelease((*instance)->rightMouseDraggedListener);
	VuoRelease((*instance)->mousePressedListener);
	VuoRelease((*instance)->mouseScrolledListener);
}
