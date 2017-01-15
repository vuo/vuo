/**
 * @file
 * VuoIconPosition C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOICONPOSITION_H
#define VUOICONPOSITION_H

/// @{
typedef void * VuoList_VuoIconPosition;
#define VuoList_VuoIconPosition_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoIconPosition VuoIconPosition
 * The position of an icon image relative to its label
 *
 * @{
 */

/**
 * The position of an icon image relative to its label
 */
typedef enum
{
	VuoIconPosition_Left,
	VuoIconPosition_Right,
	VuoIconPosition_Above,
	VuoIconPosition_Below,
	VuoIconPosition_Behind
} VuoIconPosition;

VuoIconPosition VuoIconPosition_makeFromJson(struct json_object *js);
struct json_object *VuoIconPosition_getJson(const VuoIconPosition value);
VuoList_VuoIconPosition VuoIconPosition_getAllowedValues(void);
char *VuoIconPosition_getSummary(const VuoIconPosition value);
bool VuoIconPosition_areEqual(const VuoIconPosition value1, const VuoIconPosition value2);

/**
 * Automatically generated function.
 */
///@{
VuoIconPosition VuoIconPosition_makeFromString(const char *str);
char *VuoIconPosition_getString(const VuoIconPosition value);
void VuoIconPosition_retain(VuoIconPosition value);
void VuoIconPosition_release(VuoIconPosition value);
///@}

/**
 * @}
 */

#endif // VUOICONPOSITION_H

