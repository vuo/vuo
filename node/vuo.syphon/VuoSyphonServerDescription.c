/**
 * @file
 * VuoSyphonServerDescription implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <string.h>

#include "type.h"
#include "VuoSyphonServerDescription.h"

/// @{
#ifdef VUO_COMPILER
 VuoModuleMetadata({
					"title" : "Syphon Server Description",
					"description" : "Syphon Server information.",
					"keywords" : ["syphon", "server", "pipe"],
					"version" : "1.0.0",
					"dependencies" : [
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
VuoSyphonServerDescription VuoSyphonServerDescription_valueFromJson(json_object * js)
{
	VuoSyphonServerDescription server = VuoSyphonServerDescription_make("", "", "") ;
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "serverUUID", &o))
		server.serverUUID = VuoText_valueFromJson(o);

	if (json_object_object_get_ex(js, "serverName", &o))
		server.serverName = VuoText_valueFromJson(o);

	if (json_object_object_get_ex(js, "applicationName", &o))
		server.applicationName = VuoText_valueFromJson(o);

	return server;
}

/**
 * @ingroup VuoSyphonServerDescription
 * Encodes @c value as a JSON object.
 */
json_object * VuoSyphonServerDescription_jsonFromValue(const VuoSyphonServerDescription value)
{
	json_object *js = json_object_new_object();

	json_object *uuidObject = VuoText_jsonFromValue(value.serverUUID);
	json_object_object_add(js, "serverUUID", uuidObject);

	json_object *serverName = VuoText_jsonFromValue(value.serverName);
	json_object_object_add(js, "serverName", serverName);

	json_object *applicationName = VuoText_jsonFromValue(value.applicationName);
	json_object_object_add(js, "applicationName", applicationName);

	return js;
}

/**
 * @ingroup VuoSyphonServerDescription
 * Returns a compact string representation of @c value.
 */
char * VuoSyphonServerDescription_summaryFromValue(const VuoSyphonServerDescription value)
{
	const char *format = "Syphon server \"%s\" in application \"%s\"<br>UUID: %s";

	const char *serverUUID = strlen(value.serverUUID) > 0 ? value.serverUUID : "(unknown)";
	const char *serverName = strlen(value.serverName) > 0 ? value.serverName : "(unknown)";
	const char *applicationName = strlen(value.applicationName) > 0 ? value.applicationName : "(unknown)";

	int size = snprintf(NULL,0,format,serverName,applicationName,serverUUID);
	char *valueAsString = (char *)malloc(size+1);
	snprintf(valueAsString,size+1,format,serverName,applicationName,serverUUID);
	return valueAsString;
}

/**
 * @ingroup VuoSyphonServerDescription
 * Creates a new server from the specified values.
 */
VuoSyphonServerDescription VuoSyphonServerDescription_make(VuoText serverUUID, VuoText serverName, VuoText applicationName)
{
	VuoSyphonServerDescription server;

	server.serverUUID = serverUUID;
	server.serverName = serverName;
	server.applicationName = applicationName;

	return server;
}
