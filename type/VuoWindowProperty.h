/**
 * @file
 * VuoWindowProperty C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoWindowProperty_h
#define VuoWindowProperty_h

/**
 * @ingroup VuoTypes
 * @defgroup VuoWindowProperty VuoWindowProperty
 * A window setting, such as its title, or whether it is full-screen.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoBoolean.h"
#include "VuoCursor.h"
#include "VuoInteger.h"
#include "VuoReal.h"
#include "VuoScreen.h"
#include "VuoText.h"
#include "VuoCoordinateUnit.h"
#include "VuoInteraction.h"

/**
 * The type of window property.
 *
 * @version200Changed{Added `VuoWindowProperty_Interaction`.}
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
	VuoWindowProperty_Cursor,
	VuoWindowProperty_Interaction,
	VuoWindowProperty_Level,
} VuoWindowPropertyType;

/**
 * The type of window property.
 *
 * @version200Changed{Added `VuoWindowProperty_Interaction`.}
 */
typedef enum
{
	VuoWindowLevel_Background,
	VuoWindowLevel_Normal,
	VuoWindowLevel_Floating,
} VuoWindowLevel;

/**
 * A window setting, such as its title, or whether it is full-screen.
 */
typedef struct
{
	VuoWindowPropertyType type;

	VuoCoordinateUnit unit;  ///< Points or Pixels only.

	union
	{
		VuoText title;

		struct
		{
			VuoBoolean fullScreen;
			VuoScreen screen;
		};

		struct
		{
			VuoInteger left;
			VuoInteger top;
		};

		struct
		{
			VuoInteger width;
			VuoInteger height;
		};

		VuoReal aspectRatio;

		VuoBoolean resizable;

		VuoCursor cursor;

		VuoInteraction interaction;

		VuoWindowLevel level;
	};
} VuoWindowProperty;

#include "VuoList_VuoWindowProperty.h"

VuoWindowProperty VuoWindowProperty_makeFromJson(struct json_object * js);
struct json_object * VuoWindowProperty_getJson(const VuoWindowProperty value);
char * VuoWindowProperty_getSummary(const VuoWindowProperty value);

VuoList_VuoWindowProperty VuoWindowProperty_getPropertiesWithType(const VuoList_VuoWindowProperty windowProperties, const VuoWindowPropertyType windowPropertyType);

/**
 * Automatically generated function.
 */
///@{
char * VuoWindowProperty_getString(const VuoWindowProperty value);
void VuoWindowProperty_retain(VuoWindowProperty value);
void VuoWindowProperty_release(VuoWindowProperty value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
