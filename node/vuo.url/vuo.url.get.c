/**
 * @file
 * vuo.url.get node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Get URL Values",
					  "keywords" : [ "split", "tokenize", "explode", "parts", "piece", "https" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoUrl"
					  ],
					  "node" : {
						  "exampleCompositions" : [ "ShowUrlComponents.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"name":"URL"}) url,
		VuoOutputData(VuoText) scheme,
		VuoOutputData(VuoText) user,
		VuoOutputData(VuoText) host,
		VuoOutputData(VuoInteger) port,
		VuoOutputData(VuoText) path,
		VuoOutputData(VuoText) query,
//		VuoOutputData(VuoDictionary_VuoText) queryValues
		VuoOutputData(VuoText) fragment
)
{
	if (!VuoUrl_getParts(url, scheme, user, host, port, path, query, fragment))
	{
//		VUserLog("Warning: Couldn't parse \"%s\"", url);
		*scheme = *user = *host = *path = *query = *fragment = NULL;
		*port = 0;
	}
}
