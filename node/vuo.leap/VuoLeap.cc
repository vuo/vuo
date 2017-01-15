/**
 * @file
 * VuoLeap implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

extern "C"
{
#include "VuoLeap.h"
#include "VuoLeapHand.h"
#include "VuoLeapPointable.h"
#include "VuoList_VuoLeapHand.h"
#include "VuoList_VuoLeapPointable.h"
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <Leap.h>
#pragma clang diagnostic pop

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoLeap",
					 "dependencies" : [
						 "VuoLeapFrame",
						 "VuoLeapHand",
						 "VuoLeapPointable",
						 "VuoList_VuoLeapHand",
						 "VuoList_VuoLeapPointable",
						 "VuoTransform",
						 "Leap"
					 ]
				 });
#endif
}


/**
 * Scales the specified @a milimeters into Vuo Coordinates,
 * using the width of the interaction box as a reference.
 */
static double VuoLeap_vuoDistanceFromLeapMillimeters(double millimeters, Leap::InteractionBox ibox)
{
	return (millimeters / ibox.width()) * 2.;
}

/**
 * Transforms the specified @a vector into Vuo Coordinates,
 * using the width of the interaction box as a reference.
 */
static VuoPoint3d VuoLeap_vuoPointFromLeapVector(Leap::Vector vector, Leap::InteractionBox ibox)
{
	return VuoPoint3d_make(
				VuoLeap_vuoDistanceFromLeapMillimeters(vector.x, ibox),
				VuoLeap_vuoDistanceFromLeapMillimeters(vector.y, ibox),
				VuoLeap_vuoDistanceFromLeapMillimeters(vector.z, ibox)
				);
}

/**
 *	Returns a VuoLeapTouchZone type with a Leap::Pointable::Zone enum.
 */
static VuoLeapTouchZone VuoLeap_vuoLeapTouchZoneFromLeapTouchZone(Leap::Pointable::Zone zone)
{
	switch(zone)
	{
		case Leap::Pointable::ZONE_TOUCHING:
			return VuoLeapTouchZone_Touching;

		case Leap::Pointable::ZONE_HOVERING:
			return VuoLeapTouchZone_Hovering;

		default:
		case Leap::Pointable::ZONE_NONE:
			return VuoLeapTouchZone_None;
	}
}

/**
 * Transforms the specified @a position into Vuo Coordinates,
 * using the center and width of the interaction box as a reference.
 */
static VuoPoint3d VuoLeap_vuoPointFromLeapPosition(Leap::Vector position, Leap::InteractionBox ibox)
{
	return VuoLeap_vuoPointFromLeapVector(position - ibox.center(), ibox);
}

/**
 * Creates a @c VuoPoint3d from a @c Leap::Vector.
 */
static VuoPoint3d VuoPoint3dWithLeapVector(Leap::Vector vector)
{
	return VuoPoint3d_make(vector.x, vector.y, vector.z);
}

/**
 * Listens for Leap events, and passes them along to the specified trigger port.
 */
class VuoLeapListener : public Leap::Listener {
public:

	/**
	 * Creates a listener, which calls the specified trigger function when it receives a frame.
	 */
	VuoLeapListener
	(
			void (*receivedFrame)(VuoLeapFrame frame)
	)
	{
		this->receivedFrame = receivedFrame;
	}

private:
	void (*receivedFrame)(VuoLeapFrame frame);

	virtual void onInit(const Leap::Controller&) {}
	virtual void onDisconnect(const Leap::Controller&) {}
	virtual void onExit(const Leap::Controller&) {}
	virtual void onFocusGained(const Leap::Controller&) {}
	virtual void onFocusLost(const Leap::Controller&) {}

	virtual void onConnect(const Leap::Controller &controller)
	{
		controller.enableGesture(Leap::Gesture::TYPE_CIRCLE);
		controller.enableGesture(Leap::Gesture::TYPE_KEY_TAP);
		controller.enableGesture(Leap::Gesture::TYPE_SCREEN_TAP);
		controller.enableGesture(Leap::Gesture::TYPE_SWIPE);

		controller.setPolicyFlags(Leap::Controller::POLICY_BACKGROUND_FRAMES);
	}

	virtual void onFrame(const Leap::Controller &controller)
	{
		const Leap::Frame frame = controller.frame();

		// used to normalize leap coordinates (the boolean parameter says 'no clamping' which allows for coordinates > 1)
		// https://developer.leapmotion.com/documentation/Languages/Java/API/classcom_1_1leapmotion_1_1leap_1_1_interaction_box.html#details
		Leap::InteractionBox interactionBox = frame.interactionBox();

		// https://developer.leapmotion.com/documentation/Languages/C++/Guides/Leap_Frames.html
		if(!frame.isValid())
			return;

		VuoList_VuoLeapPointable pointables = VuoListCreate_VuoLeapPointable();

		Leap::HandList allHands = frame.hands();
		Leap::PointableList allPointables = frame.pointables();

		for (Leap::PointableList::const_iterator pointable = allPointables.begin(); pointable != allPointables.end(); ++pointable)
			VuoListAppendValue_VuoLeapPointable(pointables, VuoLeapPointable_make(
													(*pointable).id(),
													(*pointable).isFinger() ? VuoLeapPointableType_Finger : VuoLeapPointableType_Tool,
													VuoLeap_vuoDistanceFromLeapMillimeters((*pointable).length(), interactionBox),
													VuoLeap_vuoDistanceFromLeapMillimeters((*pointable).width(), interactionBox),
													VuoPoint3dWithLeapVector( (*pointable).direction() ),	// leave as is
													VuoLeap_vuoPointFromLeapPosition((*pointable).stabilizedTipPosition(), interactionBox),
													VuoLeap_vuoPointFromLeapVector((*pointable).tipVelocity(), interactionBox),
													(*pointable).timeVisible(),
													(*pointable).touchDistance(),
													VuoLeap_vuoLeapTouchZoneFromLeapTouchZone((*pointable).touchZone()),
													(*pointable).isExtended()
													));


		VuoList_VuoLeapHand hands = VuoListCreate_VuoLeapHand();

		for(Leap::HandList::const_iterator hand = allHands.begin(); hand != allHands.end(); ++hand)
		{
			VuoList_VuoLeapPointable handPointables = VuoListCreate_VuoLeapPointable();

			Leap::FingerList fingers = (*hand).fingers();

			for (Leap::FingerList::const_iterator pointable = fingers.begin(); pointable != fingers.end(); ++pointable)
				VuoListAppendValue_VuoLeapPointable(handPointables, VuoLeapPointable_make(
														(*pointable).id(),
														VuoLeapPointableType_Finger,	// it's a finger.
														VuoLeap_vuoDistanceFromLeapMillimeters((*pointable).length(), interactionBox),
														VuoLeap_vuoDistanceFromLeapMillimeters((*pointable).width(), interactionBox),
														VuoPoint3dWithLeapVector( (*pointable).direction() ),
														VuoLeap_vuoPointFromLeapPosition((*pointable).stabilizedTipPosition(), interactionBox),
														VuoLeap_vuoPointFromLeapVector((*pointable).tipVelocity(), interactionBox),
														(*pointable).timeVisible(),
														(*pointable).touchDistance(),
														VuoLeap_vuoLeapTouchZoneFromLeapTouchZone((*pointable).touchZone()),
														(*pointable).isExtended()
													));

			Leap::Matrix basis = (*hand).basis();
			if ((*hand).isLeft())
			{
				// "the basis matrix will be left-handed for left hands"
				// https://developer.leapmotion.com/documentation/skeletal/cpp/api/Leap.Hand.html#cppclass_leap_1_1_hand_1a4ff50b291a30106f8306ba26d55ada76
				basis.xBasis *= -1;
			}
			Leap::Matrix basisInverse = basis.rigidInverse();
			VuoPoint3d basisPoint3d[3] = {
				VuoPoint3dWithLeapVector(basisInverse.xBasis),
				VuoPoint3dWithLeapVector(basisInverse.yBasis),
				VuoPoint3dWithLeapVector(basisInverse.zBasis)
			};

				VuoListAppendValue_VuoLeapHand(hands, VuoLeapHand_make(
													(*hand).id(),
													VuoTransform_quaternionFromBasis(basisPoint3d),
													VuoLeap_vuoPointFromLeapPosition((*hand).stabilizedPalmPosition(), interactionBox),
													VuoLeap_vuoPointFromLeapVector((*hand).palmVelocity(), interactionBox),
													VuoLeap_vuoDistanceFromLeapMillimeters((*hand).sphereRadius(), interactionBox),
													VuoLeap_vuoPointFromLeapPosition((*hand).sphereCenter(), interactionBox),
													VuoLeap_vuoDistanceFromLeapMillimeters((*hand).palmWidth(), interactionBox),
													VuoLeap_vuoPointFromLeapPosition( (*hand).wristPosition(), interactionBox ),
													(*hand).pinchStrength(),
													(*hand).grabStrength(),
													(*hand).timeVisible(),
													(*hand).isLeft(),
													(*hand).confidence(),
													handPointables)
						);
		}

		receivedFrame(VuoLeapFrame_make(frame.id(), hands, pointables));
	}
};

/**
 * Internal data for listening to a Leap controller.
 */
typedef struct _VuoLeapInternal
{
	Leap::Controller *controller;
	VuoLeapListener *listener;
} *VuoLeapInternal;

/**
 * Starts listening for Leap frames.  When a frame is received, the specified trigger function is called.
 *
 * Returns an object that can be used to stop listening.
 *
 * May be called multiple times per process, since the Leap SDK kindly allows multiple instances of @c Leap::Controller.
 */
VuoLeap VuoLeap_startListening
(
		VuoOutputTrigger(receivedFrame, VuoLeapFrame)
)
{
	VuoLeapInternal leap = (VuoLeapInternal)malloc(sizeof(struct _VuoLeapInternal));

	leap->controller = new Leap::Controller;
	leap->listener = new VuoLeapListener(receivedFrame);
	leap->controller->addListener(*leap->listener);

	return (VuoLeap)leap;
}

/**
 * Stops listening for Leap frames.
 */
void VuoLeap_stopListening(VuoLeap l)
{
	VuoLeapInternal leap = (VuoLeapInternal)l;

	leap->controller->removeListener(*leap->listener);

	delete leap->listener;
	delete leap->controller;
	delete leap;
}
