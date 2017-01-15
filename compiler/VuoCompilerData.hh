/**
 * @file
 * VuoCompilerData interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERDATA_H
#define VUOCOMPILERDATA_H

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

#endif
