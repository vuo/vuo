/**
 * @file
 * VuoCompilerDataClass implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerDataClass.hh"
#include "VuoCompilerType.hh"

/**
 * Creates a data type for a data-and-event port.
 */
VuoCompilerDataClass::VuoCompilerDataClass(string name, Type *type) :
	VuoCompilerNodeArgumentClass(name, VuoPortClass::notAPort, type)
{
	vuoType = NULL;
	details = NULL;
}

/**
 * Destructor.
 */
VuoCompilerDataClass::~VuoCompilerDataClass(void)
{
	json_object_put(details);
}

/**
 * Returns the @c VuoType for this port data, as set in @c setVuoType.
 */
VuoType * VuoCompilerDataClass::getVuoType(void)
{
	return vuoType;
}

/**
 * Sets the @c VuoType for this port data. Its @c VuoCompilerType may be null
 * (but needs to be non-null by the time the composition is compiled).
 */
void VuoCompilerDataClass::setVuoType(VuoType *vuoType)
{
	this->vuoType = vuoType;
}

/**
 * Returns the LLVM type for this port data's Vuo type.
 *
 * Overrides the implementation in @c VuoCompilerNodeArgumentClass.
 */
Type * VuoCompilerDataClass::getType(void)
{
	return vuoType->getCompiler()->getType();
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
void VuoCompilerDataClass::setDetails(struct json_object *details)
{
	this->details = details;
}

/**
 * Returns details for this port data, set in @c setDetails().
 */
json_object * VuoCompilerDataClass::getDetails(void)
{
	return details;
}
