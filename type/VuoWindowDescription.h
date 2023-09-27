/**
 * @file
 * VuoWindowDescription C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoWindowDescription_h
#define VuoWindowDescription_h

/**
 * @ingroup VuoTypes
 * @defgroup VuoWindowDescription VuoWindowDescription
 * The settings for a window, such as its title and whether it is full-screen.
 *
 * The settings do not reference any particular window. They can be applied to more than one window.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoWindowProperty.h"

/**
 * The settings for a window, such as its title and whether it is full-screen.
 *
 * For convenience the implementation is based on VuoWindowProperty, the predecessor of this type.
 *
 * @version200New
 */
typedef VuoList_VuoWindowProperty VuoWindowDescription;

VuoWindowDescription VuoWindowDescription_makeFromJson(struct json_object * js);
struct json_object * VuoWindowDescription_getJson(const VuoWindowDescription value);
char * VuoWindowDescription_getSummary(const VuoWindowDescription value);

VuoWindowDescription VuoWindowDescription_copy(const VuoWindowDescription value);
void VuoWindowDescription_setProperty(VuoWindowDescription value, VuoWindowProperty property);
VuoList_VuoWindowProperty VuoWindowDescription_getWindowProperties(const VuoWindowDescription value);

/**
 * Automatically generated function.
 */
///@{
char * VuoWindowDescription_getString(const VuoWindowDescription value);
void VuoWindowDescription_retain(VuoWindowDescription value);
void VuoWindowDescription_release(VuoWindowDescription value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
