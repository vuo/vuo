/**
 * @file
 * vuo.uuid C type definition.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once
struct json_object;

/// @{ List type.
typedef void * VuoList_VuoUuid;
#define VuoList_VuoUuid_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoUuid VuoUuid
 * Universally Unique Identifier
 *
 * @{
 */

/**
 * A Universally Unique Identifier
 *
 * @version200New
 */
typedef struct
{
	// can't return unsigned char[16] from VuoUuid_makeFromJson, so store in a struct.
	unsigned char bytes[16]; ///< 16 byte unsigned
	char blah[42]; ///< @todo https://b33p.net/kosada/node/4124
} VuoUuid;

VuoUuid VuoUuid_makeFromJson(struct json_object * js);
struct json_object * VuoUuid_getJson(const VuoUuid value);
char * VuoUuid_getSummary(const VuoUuid value);

VuoUuid VuoUuid_make();

/// @{
/**
 * Automatically generated function.
 */
VuoUuid VuoUuid_makeFromString(const char *str);
char * VuoUuid_getString(const VuoUuid value);
void VuoUuid_retain(VuoUuid value);
void VuoUuid_release(VuoUuid value);
/// @}

/**
 * Returns true if the two values are equal.
 *
 * @version200New
 */
static inline bool VuoUuid_areEqual(const VuoUuid value1, const VuoUuid value2)
{
	return 	value1.bytes[ 0] == value2.bytes[ 0] &&
			value1.bytes[ 1] == value2.bytes[ 1] &&
			value1.bytes[ 2] == value2.bytes[ 2] &&
			value1.bytes[ 3] == value2.bytes[ 3] &&
			value1.bytes[ 4] == value2.bytes[ 4] &&
			value1.bytes[ 5] == value2.bytes[ 5] &&
			value1.bytes[ 6] == value2.bytes[ 6] &&
			value1.bytes[ 7] == value2.bytes[ 7] &&
			value1.bytes[ 8] == value2.bytes[ 8] &&
			value1.bytes[ 9] == value2.bytes[ 9] &&
			value1.bytes[10] == value2.bytes[10] &&
			value1.bytes[11] == value2.bytes[11] &&
			value1.bytes[12] == value2.bytes[12] &&
			value1.bytes[13] == value2.bytes[13] &&
			value1.bytes[14] == value2.bytes[14] &&
			value1.bytes[15] == value2.bytes[15];
}

/**
 * @}
 */
