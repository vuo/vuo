/**
 * @file
 * VuoTypeStubs implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

/// @todo figure out a decent way to generate 32bit retain/release code for the types needed by framework32.

#include "VuoHeap.h"

struct json_object;

/// @{

#include "VuoBoolean.h"
void VuoBoolean_retain(VuoBoolean v) {}
void VuoBoolean_release(VuoBoolean v) {}

#include "VuoInteger.h"
void VuoInteger_retain(VuoInteger v) {}
void VuoInteger_release(VuoInteger v) {}

#include "VuoReal.h"
void VuoReal_retain(VuoReal v) {}
void VuoReal_release(VuoReal v) {}
VuoReal VuoReal_makeFromString(const char *str)
{
	json_object * js = json_tokener_parse(str);
	VuoReal value = VuoReal_makeFromJson(js);
	json_object_put(js);
	return value;
}

#include "VuoColor.h"
void VuoColor_retain(VuoColor v) {}
void VuoColor_release(VuoColor v) {}

#include "VuoPoint2d.h"
void VuoPoint2d_retain(VuoPoint2d v) {}
void VuoPoint2d_release(VuoPoint2d v) {}

#include "VuoPoint3d.h"
void VuoPoint3d_retain(VuoPoint3d v) {}
void VuoPoint3d_release(VuoPoint3d v) {}

#include "VuoPoint4d.h"
void VuoPoint4d_retain(VuoPoint4d v) {}
void VuoPoint4d_release(VuoPoint4d v) {}

#include "VuoScreen.h"
void VuoScreen_retain(VuoScreen v)
{
	VuoRetain(v.name);
}
void VuoScreen_release(VuoScreen v)
{
	VuoRelease(v.name);
}

#include "VuoImage.h"
void VuoImage_retain(VuoImage v)
{
	VuoRetain(v);
}
void VuoImage_release(VuoImage v)
{
	VuoRelease(v);
}

/// @}
