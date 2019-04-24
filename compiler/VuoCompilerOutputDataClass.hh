/**
 * @file
 * VuoCompilerOutputDataClass interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoCompilerDataClass.hh"


/**
 * The data type for a data-and-event output port.
 *
 * \see{VuoCompilerOutputData}
 */
class VuoCompilerOutputDataClass : public VuoCompilerDataClass
{
public:
	VuoCompilerOutputDataClass(string name, Type *type);
	VuoCompilerData * newData(void);
};
