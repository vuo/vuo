/**
 * @file
 * VuoTransform2d C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOTRANSFORM2D_H
#define VUOTRANSFORM2D_H

#include "VuoPoint2d.h"
#include "VuoReal.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoTransform2d VuoTransform2d
 * A 2D transformation (scale, rotation, translation).
 *
 * @{
 */

/**
 * A 2D transformation (scale, rotation, translation).
 */
typedef struct
{
	VuoPoint2d translation;
	VuoReal rotation; ///< Radians around the Z-axis.
	VuoPoint2d scale;
} VuoTransform2d;

VuoTransform2d VuoTransform2d_makeIdentity(void);
VuoTransform2d VuoTransform2d_make(VuoPoint2d translation, VuoReal rotation, VuoPoint2d scale);

VuoTransform2d VuoTransform2d_makeFromJson(struct json_object *js);
struct json_object * VuoTransform2d_getJson(const VuoTransform2d value);
char * VuoTransform2d_getSummary(const VuoTransform2d value);

/// @{
/**
 * Automatically generated function.
 */
VuoTransform2d VuoTransform2d_makeFromString(const char *str);
char * VuoTransform2d_getString(const VuoTransform2d value);
void VuoTransform2d_retain(VuoTransform2d value);
void VuoTransform2d_release(VuoTransform2d value);
/// @}

/**
 * Returns true if the transform is an identity (i.e., causes no change).
 */
static inline bool VuoTransform2d_isIdentity(const VuoTransform2d transform)
{
	const float tolerance = 0.00001f;
	return     fabs(transform.translation.x) < tolerance
			&& fabs(transform.translation.y) < tolerance
			&& fabs(transform.scale.x - 1.) < tolerance
			&& fabs(transform.scale.y - 1.) < tolerance
			&& fabs(transform.rotation) < tolerance;
}

/**
 * @}
 */

#endif
