/**
 * @file
 * VuoCompilerOutputData implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerOutputData.hh"
#include "VuoCompilerOutputDataClass.hh"

/**
 * Creates data for a data-and-event output port.
 */
VuoCompilerOutputData::VuoCompilerOutputData(VuoCompilerOutputDataClass *dataClass)
	: VuoCompilerData(dataClass)
{
}
