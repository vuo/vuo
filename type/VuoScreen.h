/**
 * @file
 * VuoScreen C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
 * How the screen should be selected from those available on the running system.
 */
typedef enum
{
	VuoScreenType_Active,
	VuoScreenType_Primary,
	VuoScreenType_Secondary,
	VuoScreenType_MatchId,
	VuoScreenType_MatchName
} VuoScreenType;

/**
 * Information about a display screen.
 */
typedef struct
{
	VuoScreenType type;
	VuoInteger id;	///< NSScreenNumber
	VuoText name;	///< e.g., `Color LCD` for a built-in MacBook Pro display

	bool isRealized;	///< True if this VuoScreen refers to a specific screen by ID, and the following values are filled in.

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

bool VuoScreen_realize(VuoScreen screen, VuoScreen *realizedScreen);

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
	VuoScreen s = {VuoScreenType_MatchName,0,name,false,{0,0},0,0,0,0};
	return s;
}

/**
 * Returns a string constant representing `type`.
 */
static inline const char * VuoScreen_cStringForType(VuoScreenType type)
{
	switch (type)
	{
		case VuoScreenType_Active:
			return "active";
		case VuoScreenType_Primary:
			return "primary";
		case VuoScreenType_Secondary:
			return "secondary";
		case VuoScreenType_MatchId:
			return "match-id";
		case VuoScreenType_MatchName:
			return "match-name";
		// -Wunreachable-code doesn't like it if we cover all possible enum values *and* specify a default.
		//default:
		//	return "primary";
	}
}

/**
 * Returns the `VuoScreenType` corresponding with the `typeString`.  If none matches, returns VuoScreenType_Primary.
 */
static inline VuoScreenType VuoScreen_typeFromCString(const char *typeString)
{
	if (strcmp(typeString,"active")==0)
		return VuoScreenType_Active;
	else if (strcmp(typeString,"primary")==0)
		return VuoScreenType_Primary;
	else if (strcmp(typeString,"secondary")==0)
		return VuoScreenType_Secondary;
	else if (strcmp(typeString,"match-id")==0)
		return VuoScreenType_MatchId;
	else if (strcmp(typeString,"match-name")==0)
		return VuoScreenType_MatchName;

	return VuoScreenType_Active;
}

/**
 * @}
 */

#endif // VUOSCREEN_H
