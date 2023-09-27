/**
 * @file
 * VuoImageSmooth interface.  Header-only, to support generic types.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VUOSMOOTH_H
#define VUOSMOOTH_H

/**
 * Instance data for smoothing a value using inertia.
 */
typedef struct _VuoSmoothInertia
{
	bool moving;
	bool updateNeededAfterSetPosition;
	VuoGenericType1 positionLastFrame;
	VuoGenericType1 velocity;
	VuoGenericType1 target;
	VuoGenericType1 positionWhenTargetChanged;
	VuoGenericType1 velocityWhenTargetChanged;
	VuoReal timeLastFrame;
	VuoReal timeWhenTargetChanged;
	VuoReal duration;
} *VuoSmoothInertia;

/**
 * Creates a new smoothing object, at rest in `initialPosition`.
 */
static VuoSmoothInertia VuoSmoothInertia_make(VuoGenericType1 initialPosition)
{
	VuoSmoothInertia s = (VuoSmoothInertia)calloc(1,sizeof(struct _VuoSmoothInertia));
	VuoRegister(s, free);
	s->positionLastFrame = initialPosition;
	return s;
}

/**
 * Warps to `position` (without smoothing) and stops.
 */
static void VuoSmoothInertia_setPosition(VuoSmoothInertia s, VuoGenericType1 position)
{
	s->positionLastFrame = position;
	s->moving = false;
	s->velocity = VuoGenericType1_makeFromJson(NULL);
	s->updateNeededAfterSetPosition = true;
}

/**
 * Changes the end value of the smooth movement to `target, and initiates/continues motion.
 */
static void VuoSmoothInertia_setTarget(VuoSmoothInertia s, VuoReal time, VuoGenericType1 target)
{
	s->target = target;
	s->positionWhenTargetChanged = s->positionLastFrame;
	s->velocityWhenTargetChanged = s->velocity;
	s->timeWhenTargetChanged = time;
	s->moving = true;
}

/**
 * Changes the total time between successive targets.
 */
static void VuoSmoothInertia_setDuration(VuoSmoothInertia s, VuoReal duration)
{
	s->duration = MAX(duration, 0.00001);
}

/**
 * Calculates the next time-step.
 *
 * `time` should be monotonically-increasing (not a delta).
 *
 * Returns true if motion is in progress (and thus an event should be fired).
 */
static bool VuoSmoothInertia_step(VuoSmoothInertia s, VuoReal time, VuoGenericType1 *calculatedPosition)
{
	bool movedDuringThisStep = false;

	if (s->updateNeededAfterSetPosition)
	{
		movedDuringThisStep = true;
		*calculatedPosition = s->positionLastFrame;
		s->updateNeededAfterSetPosition = false;
	}

	if (s->moving)
	{
		movedDuringThisStep = true;
		VuoReal timeSinceLastFrame = time - s->timeLastFrame;
		if (timeSinceLastFrame != 0)
		{
			double timeSinceTargetChanged = MIN(time - s->timeWhenTargetChanged, s->duration);
			double curviness = s->duration/(3.*timeSinceLastFrame);
			VuoGenericType1 p1 = VuoGenericType1_add(s->positionWhenTargetChanged, VuoGenericType1_multiply(s->velocityWhenTargetChanged, timeSinceLastFrame*curviness));
			*calculatedPosition = VuoGenericType1_bezier3(s->positionWhenTargetChanged, p1, s->target, s->target, timeSinceTargetChanged/s->duration);

			s->velocity = VuoGenericType1_multiply(VuoGenericType1_subtract(*calculatedPosition, s->positionLastFrame), 1./timeSinceLastFrame);
			s->positionLastFrame = *calculatedPosition;

			if (time - s->timeWhenTargetChanged > s->duration)
			{
				s->moving = false;
				s->velocity = VuoGenericType1_makeFromJson(NULL);
			}
		}
	}

	s->timeLastFrame = time;

	return movedDuringThisStep;
}

#endif
