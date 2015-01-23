/**
 * @file
 * VuoMouseButtonAction C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMOUSEBUTTONACTION_H
#define VUOMOUSEBUTTONACTION_H

#include "VuoPoint2d.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoMouseButtonAction VuoMouseButtonAction
 * A press, release, or click of one of the mouse buttons.
 *
 * @{
 */

/**
 * Type of mouse button.
 */
typedef enum
{
	VuoMouseButton_Left,
	VuoMouseButton_Middle,
	VuoMouseButton_Right

} VuoMouseButton;

/**
 * Type of action done to a mouse button.
 */
typedef enum
{
	VuoMouseButtonActionType_Press,
	VuoMouseButtonActionType_Release,
	VuoMouseButtonActionType_SingleClick,
	VuoMouseButtonActionType_DoubleClick,
	VuoMouseButtonActionType_TripleClick

} VuoMouseButtonActionType;

/**
 * A press, release, or click of one of the mouse buttons.
 */
typedef struct
{
	VuoMouseButton button;  ///< Which button was used.
	VuoMouseButtonActionType type;  ///< Which action was done to the button.
	VuoPoint2d position;  ///< The position of the pointer when the event occurred (in screen coordinates).

	char blah[42]; ///< @todo https://b33p.net/kosada/node/4124
} VuoMouseButtonAction;

VuoMouseButtonAction VuoMouseButtonAction_valueFromJson(struct json_object * js);
struct json_object * VuoMouseButtonAction_jsonFromValue(const VuoMouseButtonAction e);
char * VuoMouseButtonAction_summaryFromValue(const VuoMouseButtonAction e);

/**
 * Returns a mouse button action with the specified values.
 */
static inline VuoMouseButtonAction VuoMouseButtonAction_make(VuoMouseButton button, VuoMouseButtonActionType type, VuoPoint2d position) __attribute__((const));
static inline VuoMouseButtonAction VuoMouseButtonAction_make(VuoMouseButton button, VuoMouseButtonActionType type, VuoPoint2d position)
{
	VuoMouseButtonAction e;
	e.button = button;
	e.type = type;
	e.position = position;
	return e;
}

/// @{
/**
 * Automatically generated function.
 */
VuoMouseButtonAction VuoMouseButtonAction_valueFromString(const char *str);
char * VuoMouseButtonAction_stringFromValue(const VuoMouseButtonAction value);
/// @}

/**
 * @}
 */

#endif
