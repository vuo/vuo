/**
 * @file
 * VuoListPosition C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoListPosition_h
#define VuoListPosition_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoListPosition VuoListPosition
 * A position in a list.
 *
 * @{
 */

/**
 * A position in a list.
 */
typedef enum
{
	VuoListPosition_Beginning,
	VuoListPosition_End
} VuoListPosition;

#include "VuoList_VuoListPosition.h"

VuoListPosition VuoListPosition_makeFromJson(struct json_object * js);
struct json_object * VuoListPosition_getJson(const VuoListPosition value);
VuoList_VuoListPosition VuoListPosition_getAllowedValues(void);
char * VuoListPosition_getSummary(const VuoListPosition value);

/**
 * Automatically generated function.
 */
///@{
char * VuoListPosition_getString(const VuoListPosition value);
void VuoListPosition_retain(VuoListPosition value);
void VuoListPosition_release(VuoListPosition value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
