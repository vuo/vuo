﻿/**
 * @file
 * VuoCompilerInputData implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerInputData.hh"
#include "VuoCompilerInputDataClass.hh"

/**
 * Creates a port data instance based on the specified @c dataClass.
 */
VuoCompilerInputData::VuoCompilerInputData(VuoCompilerInputDataClass *dataClass) :
	VuoCompilerData(dataClass)
{
	setInitialValue( dataClass->getAutoSupersedesDefaultValue()?
						 dataClass->getAutoValue() :
						 dataClass->getDefaultValue() );
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
