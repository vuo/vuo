/**
 * @file
 * VuoCompilerData interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerNodeArgument.hh"

class VuoCompilerDataClass;

/**
 * The data for a data-and-event port.
 */
class VuoCompilerData : public VuoCompilerNodeArgument
{
protected:
	VuoCompilerData(VuoCompilerDataClass *dataClass);
};
