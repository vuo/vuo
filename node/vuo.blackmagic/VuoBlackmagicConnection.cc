/**
 * @file
 * VuoBlackmagicConnection implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <DeckLinkAPI.h>

extern "C"
{
#include "type.h"
#include "VuoBlackmagicConnection.h"
#include "VuoList_VuoBlackmagicConnection.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "Blackmagic Source",
	"description" : "Blackmagic video input port",
	"keywords" : [ ],
	"version" : "1.0.0",
	"dependencies" : [
		"VuoList_VuoBlackmagicConnection"
	]
});
#endif
/// @}
}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "svideo"
 * }
 */
VuoBlackmagicConnection VuoBlackmagicConnection_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoBlackmagicConnection value = VuoBlackmagicConnection_Composite;

	if (strcmp(valueAsString, "SVideo") == 0)
		value = VuoBlackmagicConnection_SVideo;
	else if (strcmp(valueAsString, "Component") == 0)
		value = VuoBlackmagicConnection_Component;
	else if (strcmp(valueAsString, "HDMI") == 0)
		value = VuoBlackmagicConnection_HDMI;
	else if (strcmp(valueAsString, "SDI") == 0)
		value = VuoBlackmagicConnection_SDI;
	else if (strcmp(valueAsString, "SDIOptical") == 0)
		value = VuoBlackmagicConnection_SDIOptical;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoBlackmagicConnection_getJson(const VuoBlackmagicConnection value)
{
	const char *valueAsString = "Composite";

	if (value == VuoBlackmagicConnection_SVideo)
		valueAsString = "SVideo";
	else if (value == VuoBlackmagicConnection_Component)
		valueAsString = "Component";
	else if (value == VuoBlackmagicConnection_HDMI)
		valueAsString = "HDMI";
	else if (value == VuoBlackmagicConnection_SDI)
		valueAsString = "SDI";
	else if (value == VuoBlackmagicConnection_SDIOptical)
		valueAsString = "SDIOptical";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoBlackmagicConnection VuoBlackmagicConnection_getAllowedValues(void)
{
	VuoList_VuoBlackmagicConnection l = VuoListCreate_VuoBlackmagicConnection();
	VuoListAppendValue_VuoBlackmagicConnection(l, VuoBlackmagicConnection_Composite);
	VuoListAppendValue_VuoBlackmagicConnection(l, VuoBlackmagicConnection_SVideo);
	VuoListAppendValue_VuoBlackmagicConnection(l, VuoBlackmagicConnection_Component);
	VuoListAppendValue_VuoBlackmagicConnection(l, VuoBlackmagicConnection_HDMI);
	VuoListAppendValue_VuoBlackmagicConnection(l, VuoBlackmagicConnection_SDI);
	VuoListAppendValue_VuoBlackmagicConnection(l, VuoBlackmagicConnection_SDIOptical);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoBlackmagicConnection_getSummary(const VuoBlackmagicConnection value)
{
	const char *valueAsString = "Composite";

	if (value == VuoBlackmagicConnection_SVideo)
		valueAsString = "S-Video";
	else if (value == VuoBlackmagicConnection_Component)
		valueAsString = "Component (Y/Pb/Pr)";
	else if (value == VuoBlackmagicConnection_HDMI)
		valueAsString = "HDMI";
	else if (value == VuoBlackmagicConnection_SDI)
		valueAsString = "SDI";
	else if (value == VuoBlackmagicConnection_SDIOptical)
		valueAsString = "SDI Optical";

	return strdup(valueAsString);
}

/**
 * Returns the `BMDVideoConnection` corresponding to the input `value`.
 */
uint32_t VuoBlackmagicConnection_getBMDVideoConnection(const VuoBlackmagicConnection value)
{
	BMDVideoConnection connection = bmdVideoConnectionComposite;

	if (value == VuoBlackmagicConnection_SVideo)
		connection = bmdVideoConnectionSVideo;
	else if (value == VuoBlackmagicConnection_Component)
		connection = bmdVideoConnectionComponent;
	else if (value == VuoBlackmagicConnection_HDMI)
		connection = bmdVideoConnectionHDMI;
	else if (value == VuoBlackmagicConnection_SDI)
		connection = bmdVideoConnectionSDI;
	else if (value == VuoBlackmagicConnection_SDIOptical)
		connection = bmdVideoConnectionOpticalSDI;

	return connection;
}

/**
 * Returns true if the two values are equal.
 */
bool VuoBlackmagicConnection_areEqual(const VuoBlackmagicConnection valueA, const VuoBlackmagicConnection valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoBlackmagicConnection_isLessThan(const VuoBlackmagicConnection valueA, const VuoBlackmagicConnection valueB)
{
	return valueA < valueB;
}
