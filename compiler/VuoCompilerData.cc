/**
 * @file
 * VuoCompilerData implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerData.hh"

#include "VuoPort.hh"

/**
 * Creates an instance of data for a data-and-event port.
 */
VuoCompilerData::VuoCompilerData(VuoCompilerDataClass *dataClass)
	: VuoCompilerNodeArgument(new VuoPort(dataClass->getBase()))
{
}

/**
 * Distinguishes the variable for the data from the variable for the event in data-and-event ports.
 */
string VuoCompilerData::getVariableBaseName(void)
{
	return VuoCompilerNodeArgument::getVariableBaseName() + "_data";
}
