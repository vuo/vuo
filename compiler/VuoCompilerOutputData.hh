/**
 * @file
 * VuoCompilerOutputData interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILEROUTPUTDATA_H
#define VUOCOMPILEROUTPUTDATA_H

#include "VuoCompilerData.hh"

class VuoCompilerOutputDataClass;

/**
 * The data for a data-and-event output port.
 *
 * \see{VuoCompilerOutputDataClass}
 */
class VuoCompilerOutputData : public VuoCompilerData
{
public:
	VuoCompilerOutputData(VuoCompilerOutputDataClass *dataClass);
};


#endif
