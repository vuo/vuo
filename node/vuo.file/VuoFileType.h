/**
 * @file
 * VuoFileType C type definition.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoText.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @{ List type.
typedef const struct VuoList_VuoFileType_struct { void *l; } * VuoList_VuoFileType;
#define VuoList_VuoFileType_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoFileType VuoFileType
 * The type of data in a file.
 *
 * @{
 */

/**
 * The type of data in a file.
 */
typedef enum
{
	VuoFileType_AnyFile,
	VuoFileType_Audio,
	VuoFileType_Image,
	VuoFileType_Mesh,
	VuoFileType_Movie,
	VuoFileType_Scene,
	VuoFileType_Folder,
	VuoFileType_Feed,
	VuoFileType_App,
	VuoFileType_Data,
	VuoFileType_JSON,
	VuoFileType_Table,
	VuoFileType_XML,
} VuoFileType;

VuoFileType VuoFileType_makeFromJson(struct json_object * js);
struct json_object * VuoFileType_getJson(const VuoFileType value);
VuoList_VuoFileType VuoFileType_getAllowedValues(void);
char * VuoFileType_getSummary(const VuoFileType value);

bool VuoFileType_isFileOfType(const VuoText path, VuoFileType fileType);
struct json_object *VuoFileType_getExtensions(VuoFileType fileType);

/**
 * Automatically generated function.
 */
///@{
char * VuoFileType_getString(const VuoFileType value);
void VuoFileType_retain(VuoFileType value);
void VuoFileType_release(VuoFileType value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
