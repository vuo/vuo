/**
 * @file
 * VuoCompilerDataClass implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerDataClass.hh"
#include "VuoCompilerType.hh"

/**
 * Creates a data type for a data-and-event port.
 */
VuoCompilerDataClass::VuoCompilerDataClass(string name, Type *type) :
	VuoCompilerNodeArgumentClass(name, VuoPortClass::notAPort, type)
{
	type = NULL;
	vuoType = NULL;
}

/**
 * Returns the @c VuoType for this port data, as set in @c setVuoType.
 */
VuoType * VuoCompilerDataClass::getVuoType(void)
{
	return vuoType;
}

/**
 * Sets the @c VuoType for this port data. Its @c VuoCompilerType may be null
 * (but needs to be non-null by the time the composition is compiled).
 */
void VuoCompilerDataClass::setVuoType(VuoType *vuoType)
{
	this->vuoType = vuoType;
}

/**
 * Returns the LLVM type for this port data's Vuo type.
 *
 * Overrides the implementation in @c VuoCompilerNodeArgumentClass.
 */
Type * VuoCompilerDataClass::getType(void)
{
	return vuoType->getCompiler()->getType();
}
