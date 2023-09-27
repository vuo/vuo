/**
 * @file
 * VuoCompilerOutputDataClass interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
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
	explicit VuoCompilerOutputDataClass(string name);
	VuoCompilerData * newData(void);
};
