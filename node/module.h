/**
 * @file
 * Prototypes for node class, type, and library module implementations.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef MODULE_H
#define MODULE_H

#include "VuoHeap.h"

#if !defined(_json_object_h_) && !defined(DOXYGEN)
	struct json_object;
	extern const char* json_object_to_json_string(struct json_object *obj);
#endif

#include "coreTypes.h"

#include "VuoLog.h"


/**
 * @ingroup DevelopingNodeClasses DevelopingTypes DevelopingLibraryModules
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
 * 					 "title" : "Display Console Window",
 * 					 "description" : "<p>Opens a window that displays console output ...</p>",
 * 					 "keywords" : [ "print", "log" ],
 * 					 "version" : "1.0.0",
 * 					 "dependencies" : [ "VuoWindow" ],
 * 					 "compatibleOperatingSystems": {
 * 						 "macosx" : { "min": "10.8" }
 * 					 },
 * 					 "node": {
 * 						 "isInterface" : true,
 * 						 "exampleCompositions" : [ "HelloWorld.vuo" ]
 * 					 }
 * 				 });
 * }
 *
 * \eg{
 * VuoModuleMetadata({
 * 					 "title" : "Add",
 * 					 "keywords" : [ "sum", "plus", "total", "+", "arithmetic", "calculate" ],
 * 					 "version" : "1.0.0",
 * 					 "genericTypes" : {
 * 						 "VuoGenericType1" : {
 * 							"defaultType" : "VuoReal",
 * 							"compatibleTypes" : [ "VuoInteger", "VuoReal" ]
 * 						 }
 * 					 },
 * 					 "node": {
 * 						 "exampleCompositions" : [ ]
 * 					 }
 * 				 });
 * }
 *
 * Pass a <a href="http://www.json.org/">JSON</a> specification that contains metadata about the node class,
 * port type, or library module.
 *
 * For any type of module, the keys in the JSON specification may optionally include:
 *   - "title" — A human-readable name for the module. It can be overridden by the user.
 *   - "description" — A description of the module's purpose. It may be formatted as plain text, HTML, or <a href="http://daringfireball.net/projects/markdown/">Markdown</a>.
 *   - "keywords" — A list of words related to the module's name. This list is used when searching for node classes in the Vuo Editor's Node Library.
 *   - "version" — A version number for the module. It should use the <a href="http://semver.org/">Semantic Versioning (SemVer)</a> scheme.
 *   - "dependencies" — A list of libraries that this module depends upon.
 *   - "compatibleOperatingSystems" — A set of operating systems on which this module can run. Unless this key is present, the module is assumed to run on all operating systems.
 *      This object contains keys for operating system names and values for the range of versions supported. Each range may specify "min", "max", or both.
 *      The operating systems and versions currently supported are:
 *      - "macosx" — "10.7", "10.8", "10.9"
 *   - "genericTypes" — Information about generic types used by this module.
 *      (This key is optional even if the module uses generic types. Currently, this key is only supported for node classes.)
 *      This object contains keys for generic type names and values for details about those types.
 *      For each generic type name, the details may optionally include:
 *      - "compatibleTypes" — A list of data types that the generic type is allowed be specialized (replaced) with.
 *        If none are given, the generic type is allowed be specialized with any data type.
 *      - "defaultType" — A data type that the generic type should be specialized with when this node class is instantiated in the Vuo Editor.
 *        If none is given, the node class is instantiated with the generic type.
 *
 * For node classes, the keys in the JSON specification may optionally include "node". Its keys may optionally include:
 *   - "isInterface" — True if this node class sends data to or receives data from somewhere external to the composition (e.g., input device, file, network). False by default.
 *   - "isDeprecated" — True if this node class should be considered deprecated. Deprecated node classes are omitted from the Vuo Editor's Node Library. False by default.
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
#include <dispatch/dispatch.h>

#include <dlfcn.h>

/**
 * Asynchronously stops the composition.
 */
static inline void VuoStopComposition(void)
{
	typedef void (*vuoStopCompositionType)(void);
	vuoStopCompositionType vuoStopComposition = (vuoStopCompositionType) dlsym(RTLD_SELF, "vuoStopComposition");
	if (!vuoStopComposition)
		vuoStopComposition = (vuoStopCompositionType) dlsym(RTLD_DEFAULT, "vuoStopComposition");
	vuoStopComposition();
}

/**
 * Returns true if nodes should apply free trial restrictions.
 */
static inline bool VuoIsTrial(void)
{
	bool **trialRestrictionsEnabled = (bool **) dlsym(RTLD_SELF, "VuoTrialRestrictionsEnabled");
	if (!trialRestrictionsEnabled)
		trialRestrictionsEnabled = (bool **) dlsym(RTLD_DEFAULT, "VuoTrialRestrictionsEnabled");
	if (!trialRestrictionsEnabled)
	{
//		VLog("Warning: Couldn't find symbol VuoTrialRestrictionsEnabled.");
		return true;
	}
	if (!*trialRestrictionsEnabled)
	{
//		VLog("Warning: VuoTrialRestrictionsEnabled isn't allocated.");
		return true;
	}
//	VLog("trialRestrictionsEnabled = %d",**trialRestrictionsEnabled);
	return **trialRestrictionsEnabled;
}

/**
 * Returns true if nodes/libraries should enable Pro features.
 */
static inline bool VuoIsPro(void)
{
	bool *proEnabled = (bool *) dlsym(RTLD_SELF, "VuoProEnabled");
	if (!proEnabled)
		proEnabled = (bool *) dlsym(RTLD_DEFAULT, "VuoProEnabled");
	if (!proEnabled)
	{
//		VLog("Warning: Couldn't find symbol VuoProEnabled.");
		return true;
	}
	return *proEnabled;
}

typedef void (*VuoCompositionFiniCallback)(void);	///< Callback prototype.

/**
 * Registers a callback to be invoked when the composition is shutting down,
 * after all nodes have been fini'd.
 *
 * `VuoCompositionFiniCallback`s are not called during livecoding reloads.
 */
static inline void VuoAddCompositionFiniCallback(VuoCompositionFiniCallback fini)
{
	typedef void (*vuoAddCompositionFiniCallbackType)(VuoCompositionFiniCallback);
	vuoAddCompositionFiniCallbackType vuoAddCompositionFiniCallback = (vuoAddCompositionFiniCallbackType) dlsym(RTLD_SELF, "vuoAddCompositionFiniCallback");
	if (!vuoAddCompositionFiniCallback)
		vuoAddCompositionFiniCallback = (vuoAddCompositionFiniCallbackType) dlsym(RTLD_DEFAULT, "vuoAddCompositionFiniCallback");
	if (vuoAddCompositionFiniCallback)
		vuoAddCompositionFiniCallback(fini);
}

#endif
