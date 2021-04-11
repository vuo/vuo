/**
 * @file
 * VuoCompilerInstanceDataClass implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerInstanceDataClass.hh"


/**
 * Creates a new type of node instance data, of data type @c type.
 */
VuoCompilerInstanceDataClass::VuoCompilerInstanceDataClass(string name, Type *type) :
	VuoCompilerNodeArgumentClass(name, VuoPortClass::notAPort)
{
	this->type = type;
}

/**
 * Returns the type passed to the `VuoInstanceData` macro in the node class's definition.
 */
Type * VuoCompilerInstanceDataClass::getType(void)
{
	return type;
}
