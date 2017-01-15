/**
 * @file
 * VuoCompilerData implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerData.hh"
#include "VuoCompilerDataClass.hh"
#include "VuoPort.hh"

/**
 * Creates an instance of data for a data-and-event port.
 */
VuoCompilerData::VuoCompilerData(VuoCompilerDataClass *dataClass)
	: VuoCompilerNodeArgument(new VuoPort(dataClass->getBase()))
{
}
