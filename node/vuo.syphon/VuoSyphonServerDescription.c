/**
 * @file
 * VuoSyphonServerDescription implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSyphonServerDescription.h"
#include "VuoBoolean.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Syphon Server",
					  "description" : "Syphon Server information.",
					  "keywords" : ["syphon", "server", "pipe"],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoBoolean",
						"VuoText"
					  ]
				  });
#endif
/// @}

/**
 * @ingroup VuoSyphonServerDescription
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "serverUUID" : "Vuo"
 *   }
 * }
 */
VuoSyphonServerDescription VuoSyphonServerDescription_makeFromJson(json_object * js)
{
	return (VuoSyphonServerDescription){
		VuoJson_getObjectValue(VuoText, js, "serverUUID", NULL),
		VuoJson_getObjectValue(VuoText, js, "serverName", NULL),
		VuoJson_getObjectValue(VuoText, js, "applicationName", NULL),
		VuoJson_getObjectValue(VuoBoolean, js, "useWildcard", false)
	};
}

/**
 * @ingroup VuoSyphonServerDescription
 * Encodes @c value as a JSON object.
 */
json_object * VuoSyphonServerDescription_getJson(const VuoSyphonServerDescription value)
{
	json_object *js = json_object_new_object();

	if (value.serverUUID)
	{
		json_object *uuidObject = VuoText_getJson(value.serverUUID);
		json_object_object_add(js, "serverUUID", uuidObject);
	}

	if (value.serverName)
	{
		json_object *serverName = VuoText_getJson(value.serverName);
		json_object_object_add(js, "serverName", serverName);
	}

	if (value.applicationName)
	{
		json_object *applicationName = VuoText_getJson(value.applicationName);
		json_object_object_add(js, "applicationName", applicationName);
	}

	json_object_object_add(js, "useWildcard", VuoBoolean_getJson(value.useWildcard));

	return js;
}

/**
 * @ingroup VuoSyphonServerDescription
 * Returns a compact string representation of @c value.
 */
char * VuoSyphonServerDescription_getSummary(const VuoSyphonServerDescription value)
{
	const char *serverUUID      = (value.serverUUID      && strlen(value.serverUUID     ) > 0) ? value.serverUUID      : "(unknown)";
	const char *serverName      = (value.serverName      && strlen(value.serverName     ) > 0) ? value.serverName      : "(unknown)";
	const char *applicationName = (value.applicationName && strlen(value.applicationName)) > 0 ? value.applicationName : "(unknown)";

	return VuoText_format("Syphon server \"%s\" in application \"%s\"<br>UUID: %s",
						  serverName, applicationName, serverUUID);
}

/**
 * @ingroup VuoSyphonServerDescription
 * Creates a new server from the specified values.
 *
 * @version200Changed{Added `useWildcard` argument.}
 */
VuoSyphonServerDescription VuoSyphonServerDescription_make(VuoText serverUUID, VuoText serverName, VuoText applicationName, bool useWildcard)
{
	VuoSyphonServerDescription server;

	server.serverUUID = serverUUID;
	server.serverName = serverName;
	server.applicationName = applicationName;
	server.useWildcard = useWildcard;

	return server;
}

/**
 * @ingroup VuoSyphonServerDescription
 * Returns true if the server descriptions are identical.
 */
bool VuoSyphonServerDescription_areEqual(const VuoSyphonServerDescription value1, const VuoSyphonServerDescription value2)
{
	return (VuoText_areEqual(value1.serverUUID, value2.serverUUID) &&
			VuoText_areEqual(value1.serverName, value2.serverName) &&
			VuoText_areEqual(value1.applicationName, value2.applicationName) &&
			value1.useWildcard == value2.useWildcard);
}

/**
 * Returns true if `a < b`.
 * @version200New
 */
bool VuoSyphonServerDescription_isLessThan(const VuoSyphonServerDescription a, const VuoSyphonServerDescription b)
{
	VuoType_returnInequality(VuoText,    a.serverUUID,      b.serverUUID);
	VuoType_returnInequality(VuoText,    a.serverName,      b.serverName);
	VuoType_returnInequality(VuoText,    a.applicationName, b.applicationName);
	VuoType_returnInequality(VuoBoolean, a.useWildcard,     b.useWildcard);
	return false;
}
