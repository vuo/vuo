/**
 * @file
 * VuoCompilerInputData implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerInputData.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoPort.hh"

/**
 * Creates a port data instance based on the specified @c dataClass.
 */
VuoCompilerInputData::VuoCompilerInputData(VuoCompilerInputDataClass *dataClass) :
	VuoCompilerData(dataClass)
{
	setInitialValue( dataClass->getDefaultValue() );
}

/**
 * Sets the initial value of the port data. This is the value it has from the time that
 * the composition begins executing until the first time the port data is overwritten.
 *
 * @param initialValueAsString String representation of the initial value.
 */
void VuoCompilerInputData::setInitialValue(string initialValueAsString)
{
	this->initialValueAsString = initialValueAsString;
}

/**
 * Returns the string representation of the initial value of the port data.
 */
string VuoCompilerInputData::getInitialValue(void)
{
	return initialValueAsString;
}
