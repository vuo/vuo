/**
 * @file
 * vuo.math.fit node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>

VuoModuleMetadata({
					  "title" : "Fit in Range",
					  "keywords" : [ "clamp", "restrict", "limit", "bound", "convert", "scale", "compressor", "calibrate", "train",
						  "agc", "automatic", "gain", "control" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ]
						  }
					  },
					  "node" : {
						  "exampleCompositions" : [ "ScaleAudioInput.vuo" ]
					  }
				  });

struct nodeInstanceData
{
	bool domainSet;
	VuoGenericType1 domainMin;
	VuoGenericType1 domainMax;
};

static void vuo_math_fit_updateDomain_VuoReal(VuoReal *domainMin, VuoReal *domainMax, VuoReal value, VuoReal tracking)
{
	if (value < *domainMin)
		*domainMin = VuoReal_lerp(*domainMin, value, tracking);
	if (value > *domainMax)
		*domainMax = VuoReal_lerp(*domainMax, value, tracking);
}
static VuoReal vuo_math_fit_VuoReal(VuoReal domainMin, VuoReal domainMax, VuoReal rangeMin, VuoReal rangeMax, VuoReal value)
{
	// If the domain is degenerate, use the center of the range.
	if (fabs(domainMax - domainMin) < 0.00001)
		return (rangeMin + rangeMax)/2;

	VuoReal position = (value - domainMin) / (domainMax - domainMin);
	return VuoReal_clamp(VuoReal_lerp(rangeMin, rangeMax, position), rangeMin, rangeMax);
}

static void vuo_math_fit_updateDomain_VuoInteger(VuoInteger *domainMin, VuoInteger *domainMax, VuoInteger value, VuoReal tracking)
{
	VuoReal domainMinReal = *domainMin;
	VuoReal domainMaxReal = *domainMax;
	vuo_math_fit_updateDomain_VuoReal(&domainMinReal, &domainMaxReal, value, tracking);
	*domainMin = domainMinReal;
	*domainMax = domainMaxReal;
}
static VuoInteger vuo_math_fit_VuoInteger(VuoInteger domainMin, VuoInteger domainMax, VuoInteger rangeMin, VuoInteger rangeMax, VuoInteger value)
{
	return vuo_math_fit_VuoReal(domainMin, domainMax, rangeMin, rangeMax, value);
}

static void vuo_math_fit_updateDomain_float(float *domainMin, float *domainMax, VuoReal value, VuoReal tracking)
{
	VuoReal domainMinReal = *domainMin;
	VuoReal domainMaxReal = *domainMax;
	vuo_math_fit_updateDomain_VuoReal(&domainMinReal, &domainMaxReal, value, tracking);
	*domainMin = domainMinReal;
	*domainMax = domainMaxReal;
}

static void vuo_math_fit_updateDomain_VuoPoint2d(VuoPoint2d *domainMin, VuoPoint2d *domainMax, VuoPoint2d value, VuoReal tracking)
{
	vuo_math_fit_updateDomain_float(&domainMin->x, &domainMax->x, value.x, tracking);
	vuo_math_fit_updateDomain_float(&domainMin->y, &domainMax->y, value.y, tracking);
}
static VuoPoint2d vuo_math_fit_VuoPoint2d(VuoPoint2d domainMin, VuoPoint2d domainMax, VuoPoint2d rangeMin, VuoPoint2d rangeMax, VuoPoint2d value)
{
	return VuoPoint2d_make(
				vuo_math_fit_VuoReal(domainMin.x, domainMax.x, rangeMin.x, rangeMax.x, value.x),
				vuo_math_fit_VuoReal(domainMin.y, domainMax.y, rangeMin.y, rangeMax.y, value.y)
				);
}

static void vuo_math_fit_updateDomain_VuoPoint3d(VuoPoint3d *domainMin, VuoPoint3d *domainMax, VuoPoint3d value, VuoReal tracking)
{
	vuo_math_fit_updateDomain_float(&domainMin->x, &domainMax->x, value.x, tracking);
	vuo_math_fit_updateDomain_float(&domainMin->y, &domainMax->y, value.y, tracking);
	vuo_math_fit_updateDomain_float(&domainMin->z, &domainMax->z, value.z, tracking);
}
static VuoPoint3d vuo_math_fit_VuoPoint3d(VuoPoint3d domainMin, VuoPoint3d domainMax, VuoPoint3d rangeMin, VuoPoint3d rangeMax, VuoPoint3d value)
{
	return VuoPoint3d_make(
				vuo_math_fit_VuoReal(domainMin.x, domainMax.x, rangeMin.x, rangeMax.x, value.x),
				vuo_math_fit_VuoReal(domainMin.y, domainMax.y, rangeMin.y, rangeMax.y, value.y),
				vuo_math_fit_VuoReal(domainMin.z, domainMax.z, rangeMin.z, rangeMax.z, value.z)
				);
}

static void vuo_math_fit_updateDomain_VuoPoint4d(VuoPoint4d *domainMin, VuoPoint4d *domainMax, VuoPoint4d value, VuoReal tracking)
{
	vuo_math_fit_updateDomain_float(&domainMin->x, &domainMax->x, value.x, tracking);
	vuo_math_fit_updateDomain_float(&domainMin->y, &domainMax->y, value.y, tracking);
	vuo_math_fit_updateDomain_float(&domainMin->z, &domainMax->z, value.z, tracking);
	vuo_math_fit_updateDomain_float(&domainMin->w, &domainMax->w, value.w, tracking);
}
static VuoPoint4d vuo_math_fit_VuoPoint4d(VuoPoint4d domainMin, VuoPoint4d domainMax, VuoPoint4d rangeMin, VuoPoint4d rangeMax, VuoPoint4d value)
{
	return VuoPoint4d_make(
				vuo_math_fit_VuoReal(domainMin.x, domainMax.x, rangeMin.x, rangeMax.x, value.x),
				vuo_math_fit_VuoReal(domainMin.y, domainMax.y, rangeMin.y, rangeMax.y, value.y),
				vuo_math_fit_VuoReal(domainMin.z, domainMax.z, rangeMin.z, rangeMax.z, value.z),
				vuo_math_fit_VuoReal(domainMin.w, domainMax.w, rangeMin.w, rangeMax.w, value.w)
				);
}

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *state = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(state, free);
	state->domainSet = false;
	return state;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) state,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":0, "VuoReal":0., "VuoPoint2d":{"x":0.,"y":0.}, "VuoPoint3d":{"x":0.,"y":0.,"z":0.}, "VuoPoint4d":{"x":0.,"y":0.,"z":0.,"w":0.}}}) value,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":1.0}) tracking,
		VuoInputEvent({"eventBlocking":"none"}) reset,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":0, "VuoReal":0., "VuoPoint2d":{"x":0.,"y":0.}, "VuoPoint3d":{"x":0.,"y":0.,"z":0.}, "VuoPoint4d":{"x":0.,"y":0.,"z":0.,"w":0.}}}) fittedMin,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":10,"VuoReal":1., "VuoPoint2d":{"x":1.,"y":1.}, "VuoPoint3d":{"x":1.,"y":1.,"z":1.}, "VuoPoint4d":{"x":1.,"y":1.,"z":1.,"w":1.}}}) fittedMax,
		VuoOutputData(VuoGenericType1) fittedValue,
		VuoOutputData(VuoGenericType1) min,
		VuoOutputData(VuoGenericType1) max
)
{
	if ((*state)->domainSet && !reset)
		vuo_math_fit_updateDomain_VuoGenericType1(&(*state)->domainMin, &(*state)->domainMax, value, VuoReal_clamp(tracking,0,1));
	else
	{
		(*state)->domainMin = (*state)->domainMax = value;
		(*state)->domainSet = true;
	}

	*fittedValue = vuo_math_fit_VuoGenericType1((*state)->domainMin, (*state)->domainMax, fittedMin, fittedMax, value);
	*min = (*state)->domainMin;
	*max = (*state)->domainMax;
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) state)
{
}
