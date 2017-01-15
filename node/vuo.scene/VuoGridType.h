/**
 * @file
 * VuoGridType C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOGRIDTYPE_H
#define VUOGRIDTYPE_H

/// @{
typedef const struct VuoList_VuoGridType_struct { void *l; } * VuoList_VuoGridType;
#define VuoList_VuoGridType_TYPE_DEFINED
/// @}


/**
 * @ingroup VuoTypes
 * @defgroup VuoGridType VuoGridType
 * Defines different ways of displaying a grid.
 *
 * @{
 */

/**
 * Defines different ways of displaying a grid.
 */
typedef enum {
	VuoGridType_Horizontal,
	VuoGridType_Vertical,
	VuoGridType_HorizontalAndVertical
} VuoGridType;

VuoGridType VuoGridType_makeFromJson(struct json_object * js);
struct json_object * VuoGridType_getJson(const VuoGridType value);
VuoList_VuoGridType VuoGridType_getAllowedValues(void);
char * VuoGridType_getSummary(const VuoGridType value);

/**
 * Automatically generated function.
 */
///@{
VuoGridType VuoGridType_makeFromString(const char *str);
char * VuoGridType_getString(const VuoGridType value);
void VuoGridType_retain(VuoGridType value);
void VuoGridType_release(VuoGridType value);
///@}

/**
 * @}
 */

#endif // VuoGridType_H
