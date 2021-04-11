/**
 * @file
 * Prototypes for node class, type, and library module implementations.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(VUO_STRINGIFY)
	/// Creates a C string from raw text (saves having to doublequote-escape and/or use backslashes).
	#define VUO_STRINGIFY(...) #__VA_ARGS__
#endif

#include "VuoHeap.h"

#if !defined(_json_object_h_) && !defined(DOXYGEN)
	struct json_object;
	extern const char* json_object_to_json_string(struct json_object *obj);
#endif

#include "coreTypes.h"


/**
 * @ingroup DevelopingNodeClasses
 * @defgroup VuoModuleMetadata Module Metadata
 * Name, version, and behavior information to be defined by node classes, port types, and library modules.
 *
 * @{
 */

/**
 * Name, version, and behavior information to be defined by node classes, port types, and library modules.
 *
 * \eg{
 * VuoModuleMetadata({
 *     "title" : "Display Console Window",
 *     "description" : "<p>Opens a window that displays console output ...</p>",
 *     "keywords" : [ "print", "log" ],
 *     "version" : "1.0.0",
 *     "dependencies" : [ "VuoWindow" ],
 *     "compatibility": {
 *         "macos": { "min": "10.13" }
 *     },
 *     "node": {
 *         "exampleCompositions" : [ "HelloWorld.vuo" ]
 *     }
 * });
 * }
 *
 * \eg{
 * VuoModuleMetadata({
 *     "title" : "Add",
 *     "keywords" : [ "sum", "plus", "total", "+", "arithmetic", "calculate" ],
 *     "version" : "1.0.0",
 *     "genericTypes" : {
 *         "VuoGenericType1" : {
 *             "defaultType" : "VuoReal",
 *             "compatibleTypes" : [ "VuoInteger", "VuoReal" ]
 *         }
 *     },
 *     "node": {
 *         "exampleCompositions" : [ ]
 *     }
 * });
 * }
 *
 * Pass a <a href="http://www.json.org/">JSON</a> specification that contains metadata about the node class,
 * port type, or library module.
 *
 * For any type of module, the keys in the JSON specification may optionally include:
 *   - "title" — A human-readable name for the module. It can be overridden by the user.
 *   - "description" — A description of the module's purpose. It may be formatted as plain text, HTML, or <a href="https://daringfireball.net/projects/markdown/basics">Markdown</a>.
 *   - "keywords" — A list of words related to the module's name. This list is used when searching for node classes in the Vuo editor's Node Library.
 *   - "version" — A version number for the module. It should use the <a href="http://semver.org/">Semantic Versioning (SemVer)</a> scheme.
 *   - "dependencies" — A list of libraries that this module depends upon.
 *   - "compatibility" — A set of operating systems on which this module can run. Unless this key is present, the module is assumed to run on all operating systems and CPU architectures.
 *      This object contains keys for operating system names and values for the range of versions and CPU architectures supported. Each range may specify "min", "max", "arch", or any combination.
 *      The operating systems and versions currently supported are:
 *      - "macos" — "10.11", "10.12", "10.13", "10.14", "10.15", "11.0"
 *      The CPU architectures currently supported are:
 *      - "arch" — "x86_64", "arm64"
 *   - "genericTypes" — Information about generic types used by this module.
 *      (This key is optional even if the module uses generic types. Currently, this key is only supported for node classes.)
 *      This object contains keys for generic type names and values for details about those types.
 *      For each generic type name, the details may optionally include:
 *      - "compatibleTypes" — A list of data types that the generic type is allowed be specialized (replaced) with.
 *        If none are given, the generic type is allowed be specialized with any data type.
 *      - "defaultType" — A data type that the generic type should be specialized with when this node class is instantiated in the Vuo editor.
 *        If none is given, the node class is instantiated with the generic type.
 *
 * For node classes, the keys in the JSON specification may optionally include "node". Its keys may optionally include:
 *   - "isDeprecated" — True if this node class should be considered deprecated. Deprecated node classes are omitted from the Vuo editor's Node Library. False by default.
 *   - "exampleCompositions" — A list of example compositions that demonstrate this node class. The example compositions and the node class must be packaged together in a node set.
 *
 * @see DevelopingNodeClasses
 * @see DevelopingTypes
 * @see DevelopingLibraryModules
 */
#define VuoModuleMetadata(...) extern const char *moduleDetails; const char *moduleDetails = #__VA_ARGS__

/**
 * @}
 */


#ifndef DISPATCH_RETURNS_RETAINED_BLOCK
	/// Disable DISPATCH_RETURNS_RETAINED_BLOCK, which emits warnings on Mac OS 10.10.
	/// https://b33p.net/kosada/node/9139
	#define DISPATCH_RETURNS_RETAINED_BLOCK
#endif
#include "VuoMacOSSDKWorkaround.h"
#include <dispatch/dispatch.h>

/**
 * Callback prototype for @ref VuoAddCompositionFiniCallback().
 */
typedef void (*VuoCompositionFiniCallback)(void);

/**
 * Returns the directory that nodes should use to resolve relative paths.
 */
const char *VuoGetWorkingDirectory(void);

const char *VuoGetFrameworkPath(void);
const char *VuoGetRunnerFrameworkPath(void);

/**
 * Returns the process ID of the runner that started the composition.
 */
pid_t VuoGetRunnerPid(void);

/**
 * Asynchronously stops the composition.
 */
void VuoStopComposition(void);

/**
 * Asynchronously stops the composition.
 *
 * @ref VuoStopComposition() is typically preferable to this function, since this function
 * doesn't handle multiple compositions running in the process.
 */
void VuoStopCurrentComposition(void);

/**
 * Registers a callback to be invoked when the composition is shutting down,
 * after `nodeInstanceFini()` has been called for all nodes.
 *
 * `VuoCompositionFiniCallback`s are not called during livecoding reloads.
 */
void VuoAddCompositionFiniCallback(VuoCompositionFiniCallback fini);

/**
 * Temporarily disables automatic termination.
 *
 * When a composition is asked to shut down, a watchdog timer waits a few seconds then force-quits
 * if it hasn't cleanly shut down by then. This disables that watchdog.
 *
 * Call this before entering a section where it would be undesirable to have the composition
 * automatically force-quit, such as when saving a movie file that needs to be finalized.
 *
 * When the work is over, call @ref VuoEnableTermination().
 */
void VuoDisableTermination(void);

/**
 * Resumes automatic termination after a call to @ref VuoDisableTermination().
 */
void VuoEnableTermination(void);

/**
 * Returns true if this composition should show the Vuo Community Edition splash window.
 */
bool VuoShouldShowSplashWindow(void);

bool VuoIsPro(void);

bool VuoProcessorSupportsAVX2(void);

/**
 * The standard Vuo graphics window width, in points.
 * @version200New
 */
extern const int VuoGraphicsWindowDefaultWidth;

/**
 * If `js` is a JSON object that has `key`,
 * this macro decodes `key` as `type` and returns it.
 * Otherwise, returns a default value (the last argument).
 *
 * This may be useful for implementing `Vuo*_makeFromJson()` methods for compound types.
 *
 * @hideinitializer
 *
 * @eg{
 * json_object *js = json_object_new_object();
 * json_object_object_add(js, "answer", json_object_new_double(42));
 *
 * myObject.answer = VuoJson_getObjectValue(VuoReal, js, "answer", 0));
 * }
 *
 * @version200New
 */
#define VuoJson_getObjectValue(type, js, key, ...) \
(^{                                                \
	json_object *o;                                \
	if (json_object_object_get_ex(js, key, &o))    \
		return type ## _makeFromJson(o);           \
	else                                           \
		return (type)__VA_ARGS__;                  \
}())

/**
 * Compares using the specified `type`'s `Vuo*_isLessThan()` method,
 * returning if not equal, or falling through if equal.
 *
 * This may be useful for implementing `Vuo*_isLessThan()` methods for compound types.
 *
 * @hideinitializer
 *
 * @eg{
 * VuoType_returnInequality(VuoText, myObject.question, thatObject.question);
 * VuoType_returnInequality(VuoReal, myObject.answer, thatObject.answer);
 * }
 *
 * @version200New
 */
#define VuoType_returnInequality(type, a, b)    \
	if (type ## _isLessThan(a, b)) return true; \
	if (type ## _isLessThan(b, a)) return false;

#ifdef __cplusplus
}
#endif
