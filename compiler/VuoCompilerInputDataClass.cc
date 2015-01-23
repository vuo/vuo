/**
 * @file
 * VuoCompilerInputDataClass implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json/json.h>
#pragma clang diagnostic pop

#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputData.hh"


/**
 * Creates a data type for a data-and-event input port.
 *
 * The default data value for instances of this data type is not set. Set it with @c setDefaultValue.
 */
VuoCompilerInputDataClass::VuoCompilerInputDataClass(string name, Type *type, bool twoParameters) :
	VuoCompilerDataClass(name, type)
{
	this->twoParameters = twoParameters;
	this->details = NULL;
}

/**
 * Creates a data instance based on this data type.
 */
VuoCompilerData * VuoCompilerInputDataClass::newData(void)
{
	return new VuoCompilerInputData(this);
}

/**
 * Sets details for this port data.
 *
 * @eg{
 * {
 *   "default":10,
 *   "suggestedMin":0,
 *   "suggestedMax":100
 * }
 * }
 *
 * @eg{
 * {
 *   "default":{"x":-0.5,"y":0.5}
 * }
 * }
 */
void VuoCompilerInputDataClass::setDetails(struct json_object *details)
{
	this->details = details;
}

/**
 * Returns details for this port data, set in @c setDetails().
 */
json_object * VuoCompilerInputDataClass::getDetails(void)
{
	return details;
}

/**
 * Returns the string representation of the default initial value of port data instantiated
 * from this data type.
 */
string VuoCompilerInputDataClass::getDefaultValue(void)
{
	if (details)
	{
		json_object *value = NULL;
		if (json_object_object_get_ex(details, "default", &value))
			return json_object_to_json_string_ext(value, JSON_C_TO_STRING_PLAIN);
	}

	return "";
}

/**
 * Returns true if, in the node event function, Clang converts this one parameter in source code
 * to two parameters in the compiled bitcode.
 */
bool VuoCompilerInputDataClass::isLoweredToTwoParameters(void)
{
	return twoParameters;
}
