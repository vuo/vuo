/**
 * @file
 * VuoSyphonServerDescription implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
	VuoSyphonServerDescription server = {NULL, NULL, NULL};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "serverUUID", &o))
		server.serverUUID = VuoText_makeFromJson(o);

	if (json_object_object_get_ex(js, "serverName", &o))
		server.serverName = VuoText_makeFromJson(o);

	if (json_object_object_get_ex(js, "applicationName", &o))
		server.applicationName = VuoText_makeFromJson(o);

	return server;
}

/**
 * @ingroup VuoSyphonServerDescription
 * Encodes @c value as a JSON object.
 */
json_object * VuoSyphonServerDescription_getJson(const VuoSyphonServerDescription value)
{
	json_object *js = json_object_new_object();

	json_object *uuidObject = VuoText_getJson(value.serverUUID);
	json_object_object_add(js, "serverUUID", uuidObject);

	json_object *serverName = VuoText_getJson(value.serverName);
	json_object_object_add(js, "serverName", serverName);

	json_object *applicationName = VuoText_getJson(value.applicationName);
	json_object_object_add(js, "applicationName", applicationName);

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
 */
VuoSyphonServerDescription VuoSyphonServerDescription_make(VuoText serverUUID, VuoText serverName, VuoText applicationName)
{
	VuoSyphonServerDescription server;

	server.serverUUID = serverUUID;
	server.serverName = serverName;
	server.applicationName = applicationName;

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
			VuoText_areEqual(value1.applicationName, value2.applicationName));
}
