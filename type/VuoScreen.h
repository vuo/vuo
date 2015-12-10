/**
 * @file
 * VuoScreen C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOSCREEN_H
#define VUOSCREEN_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoScreen VuoScreen
 * Information about a display screen.
 *
 * @{
 */

#include "VuoInteger.h"
#include "VuoPoint2d.h"
#include "VuoText.h"

/**
 * Information about a display screen.
 */
typedef struct
{
	VuoInteger id;	///< If `id` is non-negative, use the specified device identifier.
	VuoText name;	///< If `id` is negative, use the first device whose name contains `name` (e.g., `Color LCD` for a built-in MacBook Pro display).

	VuoPoint2d topLeft;	// In points (not pixels).

	VuoInteger width;	// In points (not pixels).
	VuoInteger height;

	VuoInteger dpiHorizontal;	// Pixels per inch horizontally — e.g., 72 for typical displays, 144 for retina
	VuoInteger dpiVertical;
} VuoScreen;

VuoScreen VuoScreen_makeFromJson(struct json_object * js);
struct json_object * VuoScreen_getJson(const VuoScreen value);
char *VuoScreen_getSummary(const VuoScreen value);
bool VuoScreen_areEqual(VuoScreen value1, VuoScreen value2);

/**
 * Automatically generated function.
 */
///@{
VuoScreen VuoScreen_makeFromString(const char *str);
char * VuoScreen_getString(const VuoScreen value);
void VuoScreen_retain(VuoScreen value);
void VuoScreen_release(VuoScreen value);
///@}

/**
 * Returns a screen with the specified name.
 */
static inline VuoScreen VuoScreen_makeFromName(VuoText name) __attribute__((const));
static inline VuoScreen VuoScreen_makeFromName(VuoText name)
{
	VuoScreen s = {-1,name,{0,0},0,0,0,0};
	return s;
}

/**
 * @}
 */

#endif // VUOSCREEN_H
