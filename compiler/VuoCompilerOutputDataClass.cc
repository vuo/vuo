/**
 * @file
 * VuoCompilerOutputDataClass implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerOutputDataClass.hh"
#include "VuoCompilerOutputData.hh"


/**
 * Creates a data type for a data-and-event output port.
 */
VuoCompilerOutputDataClass::VuoCompilerOutputDataClass(string name, Type *type) :
	VuoCompilerDataClass(name, type)
{
}

/**
 * Creates data based on this data type.
 */
VuoCompilerData * VuoCompilerOutputDataClass::newData(void)
{
	return new VuoCompilerOutputData(this);
}
