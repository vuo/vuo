/**
 * @file
 * VuoCompilerInstanceDataClass implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerInstanceDataClass.hh"


/**
 * Creates a new type of node instance data, of data type @c type.
 */
VuoCompilerInstanceDataClass::VuoCompilerInstanceDataClass(string name, Type *type) :
	VuoCompilerNodeArgumentClass(name, VuoPortClass::notAPort, type)
{
}
