/**
 * @file
 * Prototypes for type implementations.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "module.h"

#ifdef __cplusplus
extern "C" {
#endif

// Headers commonly used by type implementations

#if (__clang_major__ == 3 && __clang_minor__ >= 2) || __clang_major__ > 3
	#define VUO_CLANG_32_OR_LATER
#endif

#pragma clang diagnostic push
#ifdef VUO_CLANG_32_OR_LATER
	#pragma clang diagnostic ignored "-Wdocumentation"
#endif
#include "json-c/json.h"
#pragma clang diagnostic pop


/**
 * @defgroup VuoTypes Built-in Types
 * Types defined by Vuo Core, for use in both Vuo Graph Language and C code.
 */

/**
 * Returns a new `type` instance initialized with the JSON-formatted `const char *valueAsString`.
 *
 * For heap-allocated types, the returned value has a retain count of 1.
 *
 * @eg{
 * VuoList_VuoInteger list = VuoMakeRetainedFromString(text, VuoList_VuoInteger);
 * VuoDefer(^{ VuoList_VuoInteger_release(list); });
 * }
 */
#define VuoMakeRetainedFromString(valueAsString, type) ({                       \
    json_object *js = valueAsString ? json_tokener_parse(valueAsString) : NULL; \
    type variable = type ## _makeFromJson(js);                                  \
    type ## _retain(variable);                                                  \
    json_object_put(js);                                                        \
    variable;                                                                   \
})

#ifdef DOXYGEN

/**
 * @addtogroup DevelopingTypes
 * @{
 */


/**
 * @defgroup VuoTypeMethods Type Methods
 * Functions to serialize, unserialize, and summarize values of the type. Replace "MyType" with the name of your type.
 *
 * @{
 */

/**
 * Creates a new @c MyType value by unserializing a JSON-C object.
 *
 * This function is required.
 *
 * When implementing this function — For heap-allocated types, you should register the value with VuoRegister().
 *      In addition, if the value was serialized by MyType_getJson() to a format that can only be used within the same
 *      process, you should call `json_object_set_userdata()` and pass a callback that releases the unserialized value
 *      with MyType_release().
 *
 * When calling this function — You should subsequently retain the return value with MyType_retain() and then release
 *      @a js with `json_object_put()`, in that order. This is important when working with values serialized for use
 *      within the same process because `json_object_put()` decrements the reference count of the value
 *      (thanks to the `json_object_set_userdata()` callback mentioned above).
 *
 * @eg{
 * MyType value = MyType_makeFromJson(js);
 * MyType_retain(value);
 * json_object_put(js);
 * // … use value …
 * MyType_release(value);
 * }
 *
 * @param js A JSON-C object from MyType_getJson() (or MyType_getInterprocessJson() if it exists).
 * @return The unserialized value. For heap-allocated types, this value has been registered. The caller is responsible
 *      for retaining and releasing it.
 */
MyType MyType_makeFromJson(json_object *js);

/**
 * Serializes a @c MyType value to a JSON-C object.
 *
 * This function is required.
 *
 * If this function serializes the value in a format that won't allow another process to unserialize it
 * (for example, if the serialization contains a memory address), then MyType_getInterprocessJson()
 * needs to be implemented, too.
 *
 * When implementing this function — If @a value is being serialized in a format that can only be used within
 *       the same process, then you should retain @a value with MyType_retain().
 *
 * @param value The value to serialize. May be null.
 * @return A JSON-C object. It should have a format accepted by MyType_makeFromJson(). The caller is responsible for
 *       releasing it with `json_object_put()`.
 */
json_object * MyType_getJson(const MyType value);

/**
 * Serializes a @c MyType value to a JSON-formatted string, in a format that will allow
 * another process to unserialize it. If this function is implemented, then MyType_makeFromJson()
 * needs to handle both the format returned by this function and the format returned by MyType_getJson().
 *
 * This function is optional.
 *
 * @param value The value to serialize. May be null.
 * @return A JSON-C object. It should have a format accepted by MyType_makeFromJson(). The caller is responsible for
 *       releasing it with `json_object_put()`.
 *
 * @see VuoImage
 */
struct json_object * MyType_getInterprocessJson(const MyType value);

/**
 * Returns a list of values that instances of this type can have.
 *
 * The values returned by this function are shown in input editor menus, in the order specified.
 *
 * The type may have other values not returned by this function (for example, if the values were allowed
 * in older versions of the type, but are not allowed now).
 *
 * This function is required for enum types,
 * and should only be defined for types whose `sizeof() = 4` (`enum`, `int64_t`, `void *`, `char *`, etc).
 */
VuoList_MyType MyType_getAllowedValues(void);

/**
 * Returns a brief description of a value.
 *
 * This function is required.
 *
 * For enum types, the string returned by this function is shown in input editor menus.
 *
 * @param value The value to summarize. May be null.
 * @return The summary. It should be heap-allocated; the caller is responsible for freeing it.
 */
char * MyType_getSummary(const MyType value);

/**
 * Serializes a @c MyType value to a JSON-formatted string. Calls MyType_getJson().
 *
 * This function is automatically generated by the Vuo compiler. Do not implement it.
 * If this function needs to be called by @c MyType or other code, then declare it in @c MyType.
 *
 * @param value The value to serialize.
 * @return A JSON-formatted string representation of the value. The caller is responsible for freeing the string.
 */
char * MyType_getString(const MyType value);

/**
 * Serializes a @c MyType value to a JSON-formatted string, in a format that will allow
 * another process to unserialize it. Calls MyType_getInterprocessJson().
 *
 * This function is automatically generated by the Vuo compiler (but only if MyType_getInterprocessJson() exists).
 * Do not implement it. If this function needs to be called by @c MyType or other code, then declare it in @c MyType.
 *
 * @param value The value to serialize.
 * @return A JSON-formatted string representation of the value. The caller is responsible for freeing the string.
 */
char * MyType_getInterprocessString(const MyType value);

/**
 * Increments the reference count of a value or its fields, if needed.
 *
 *   - If @c MyType is a pointer, this function calls VuoRetain() on @a value.
 *   - If @c MyType is a struct, this function calls VuoRetain() on each field of @a value that is a pointer.
 *   - Otherwise, this function does nothing.
 *
 * This function is automatically generated by the Vuo compiler. Do not implement it.
 * If this function needs to be called by @c MyType or other code, then declare it in @c MyType.
 */
void MyType_retain(const MyType value);

/**
 * Decrements the reference count of a value or its fields, if needed.
 *
 *   - If @c MyType is a pointer, this function calls VuoRelease() on @a value.
 *   - If @c MyType is a struct, this function calls VuoRelease() on each field of @a value that is a pointer.
 *   - Otherwise, this function does nothing.
 *
 * This function is automatically generated by the Vuo compiler. Do not implement it.
 * If this function needs to be called by @c MyType or other code, then declare it in @c MyType.
 */
void MyType_release(const MyType value);

/**
 * @}
 */


/**
 * @}
 */

#endif

#ifdef __cplusplus
}
#endif
