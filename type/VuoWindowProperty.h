/**
 * @file
 * VuoWindowProperty C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOWINDOWPROPERTY_H
#define VUOWINDOWPROPERTY_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoWindowProperty VuoWindowProperty
 * A window setting, such as its title, or whether it is full-screen.
 *
 * @{
 */

#include "VuoBoolean.h"
#include "VuoCursor.h"
#include "VuoInteger.h"
#include "VuoReal.h"
#include "VuoScreen.h"
#include "VuoText.h"
#include "VuoCoordinateUnit.h"

/**
 * The type of window property.
 */
typedef enum
{
	VuoWindowProperty_Title,
	VuoWindowProperty_FullScreen,
	VuoWindowProperty_Position,
	VuoWindowProperty_Size,
	VuoWindowProperty_AspectRatio,
	VuoWindowProperty_AspectRatioReset,
	VuoWindowProperty_Resizable,
	VuoWindowProperty_Cursor
} VuoWindowPropertyType;

/**
 * A window setting, such as its title, or whether it is full-screen.
 */
typedef struct
{
	VuoWindowPropertyType type;

	VuoText title;

	VuoBoolean fullScreen;
	VuoScreen screen;

	VuoCoordinateUnit unit;
	VuoInteger left;
	VuoInteger top;

	VuoInteger width;
	VuoInteger height;

	VuoReal aspectRatio;

	VuoBoolean resizable;

	VuoCursor cursor;
} VuoWindowProperty;

VuoWindowProperty VuoWindowProperty_makeFromJson(struct json_object * js);
struct json_object * VuoWindowProperty_getJson(const VuoWindowProperty value);
char * VuoWindowProperty_getSummary(const VuoWindowProperty value);

/**
 * Automatically generated function.
 */
///@{
VuoWindowProperty VuoWindowProperty_makeFromString(const char *str);
char * VuoWindowProperty_getString(const VuoWindowProperty value);
void VuoWindowProperty_retain(VuoWindowProperty value);
void VuoWindowProperty_release(VuoWindowProperty value);
///@}

/**
 * @}
 */

#endif // VUOWINDOWPROPERTY_H

